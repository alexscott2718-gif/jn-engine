from PySide6.QtCore import QSize, Qt, QUrl
from PySide6.QtGui import QAction, QIcon, QKeySequence, QPixmap
from PySide6.QtWidgets import (
    QApplication,
    QCheckBox,
    QLabel,
    QMainWindow,
    QStatusBar,
    QToolBar,
    QWidget,
    QHBoxLayout,
    QVBoxLayout,
    QTreeWidget,
    QTreeWidgetItem,
    QPushButton,
    QStackedWidget,
    QSlider,
    QFileDialog,
    QMessageBox,
)
from PySide6.QtMultimedia import QMediaPlayer, QAudioOutput
from PySide6.QtMultimediaWidgets import QVideoWidget
import sys
import struct
import os
import zlib
import io

# Import Canvas and 3D mesh readers from the utility modules
try:
    from PIL import Image
except ImportError:
    Image = None

class OMTCanvasDecoder:
    """Decoder for OMT Canvas (texture) chunks"""
    
    @staticmethod
    def decode_canvas(data: bytes):
        """Decode canvas chunk data and return PIL Image"""
        if not Image:
            return None
            
        try:
            f = io.BytesIO(data)
            sig = f.read(4)
            if sig not in (b'OmCv', b'OmMC'):
                return None
            
            attr = struct.unpack('>I', f.read(4))[0]
            hdrtype = f.read(4)
            if hdrtype != b'OmGW':
                return None
            
            width = struct.unpack('>H', f.read(2))[0]
            height = struct.unpack('>H', f.read(2))[0]
            depth = struct.unpack('>H', f.read(2))[0]
            compression = struct.unpack('B', f.read(1))[0]
            version = struct.unpack('B', f.read(1))[0]
            nbytes = struct.unpack('>H', f.read(2))[0]
            
            # Read transparency
            dotransparent = struct.unpack('>h', f.read(2))[0] != 0
            transp_packed = None
            if dotransparent:
                if version == 0:
                    r = struct.unpack('>H', f.read(2))[0]
                    g = struct.unpack('>H', f.read(2))[0]
                    b = struct.unpack('>H', f.read(2))[0]
                    transp_packed = (r, g, b)
                else:
                    transp_packed = struct.unpack('>I', f.read(4))[0]
            
            # Read palette
            dopalette = struct.unpack('>h', f.read(2))[0] != 0
            palette = None
            if dopalette:
                hdr = f.read(4)
                size = struct.unpack('>I', f.read(4))[0]
                palette = []
                for _ in range(size):
                    r = struct.unpack('>H', f.read(2))[0]
                    g = struct.unpack('>H', f.read(2))[0]
                    b = struct.unpack('>H', f.read(2))[0]
                    palette.append((r, g, b))
                if hdr == b'OPa2':
                    f.read(16 * 16 * 16)
            
            # Read compressed buffer
            bufsize = struct.unpack('>I', f.read(4))[0]
            compressed = f.read(bufsize)
            
            # Decompress scanlines
            unit_size = {0: 1, 1: 2, 2: 4}[compression]
            pixbuf = bytearray(nbytes * height)
            src_pos = 0
            dst_pos = 0
            
            for y in range(height):
                if src_pos + 2 > len(compressed):
                    break
                nbyter = struct.unpack_from('>H', compressed, src_pos)[0]
                src_pos += 2
                if src_pos + nbyter > len(compressed):
                    break
                line_data = bytes(compressed[src_pos:src_pos + nbyter])
                src_pos += nbyter
                
                # RLE decompress
                expanded = OMTCanvasDecoder._rle_unpack(line_data, compression, nbytes)
                pixbuf[dst_pos:dst_pos + nbytes] = expanded
                dst_pos += nbytes
            
            # Convert to RGBA
            rgba = bytearray()
            if depth == 16:
                for y in range(height):
                    row = pixbuf[y * nbytes:(y + 1) * nbytes]
                    for x in range(width):
                        off = x * 2
                        v = struct.unpack_from('>H', row, off)[0]
                        r = ((v >> 7) & 0xF8) | ((v >> 12) & 0x07)
                        g = ((v >> 2) & 0xF8) | ((v >> 7) & 0x07)
                        b = ((v << 3) & 0xF8) | ((v >> 2) & 0x07)
                        a = 0 if (transp_packed is not None and v == transp_packed) else 255
                        rgba.extend((r, g, b, a))
            elif depth == 32:
                for y in range(height):
                    row = pixbuf[y * nbytes:(y + 1) * nbytes]
                    for x in range(width):
                        off = x * 4
                        v = struct.unpack_from('>I', row, off)[0]
                        r = (v >> 16) & 0xFF
                        g = (v >> 8) & 0xFF
                        b = v & 0xFF
                        a = 0 if (transp_packed is not None and v == transp_packed) else 255
                        rgba.extend((r, g, b, a))
            elif depth == 8 and palette:
                for y in range(height):
                    row = pixbuf[y * nbytes:(y + 1) * nbytes]
                    for px in row[:width]:
                        if px < len(palette):
                            r16, g16, b16 = palette[px]
                            rgba.extend(((r16 >> 8) & 0xFF, (g16 >> 8) & 0xFF, (b16 >> 8) & 0xFF, 255))
            
            img = Image.frombytes('RGBA', (width, height), bytes(rgba))
            return img
        except Exception as e:
            print(f"Canvas decode error: {e}")
            return None
    
    @staticmethod
    def _rle_unpack(src: bytes, pmode: int, dest_bytes: int):
        """RLE decompression"""
        unit_size = {0: 1, 1: 2, 2: 4}[pmode]
        out = bytearray()
        pos = 0
        
        while len(out) < dest_bytes and pos < len(src):
            if pos + unit_size > len(src):
                break
            
            raw_ctrl = src[pos:pos + unit_size]
            pos += unit_size
            control = struct.unpack('b', raw_ctrl[:1])[0]
            
            if control >= 0:
                count = control + 1
                bytes_needed = count * unit_size
                if pos + bytes_needed > len(src):
                    break
                out.extend(src[pos:pos + bytes_needed])
                pos += bytes_needed
            else:
                if control == -128 and unit_size == 1:
                    continue
                count = -control + 1
                if pos + unit_size > len(src):
                    break
                unit = src[pos:pos + unit_size]
                pos += unit_size
                out.extend(unit * count)
        
        if len(out) > dest_bytes:
            out = out[:dest_bytes]
        return out


class OMT3DMeshDecoder:
    """Decoder for OMT 3D mesh chunks"""
    
    @staticmethod
    def decode_3dsh(data: bytes, endian: str = '>'):
        """Decode 3D shape chunk data"""
        try:
            if not data.startswith(b'3DSP'):
                return None
            
            offset = 4
            version = struct.unpack_from(endian + 'I', data, offset)[0]
            offset += 4
            
            if version > 5:
                return None
            
            # Skip bounding spheres
            for _ in range(8):
                offset += 4
            
            # Skip custom far clip if version < 3
            if 0 < version < 3:
                offset += 4 + 2
            
            # Read vertices
            vcount = struct.unpack_from(endian + 'I', data, offset)[0]
            offset += 4
            vertices = []
            for _ in range(vcount):
                x = struct.unpack_from(endian + 'f', data, offset)[0]
                y = struct.unpack_from(endian + 'f', data, offset + 4)[0]
                z = struct.unpack_from(endian + 'f', data, offset + 8)[0]
                vertices.append((x, y, z))
                offset += 12
            
            # Read polygons count
            pcount = struct.unpack_from(endian + 'I', data, offset)[0]
            offset += 4
            
            mesh_info = {
                'version': version,
                'vertices': vertices,
                'polygon_count': pcount,
                'offset': offset
            }
            return mesh_info
        except Exception as e:
            print(f"3D mesh decode error: {e}")
            return None

class OMTFormatParser:
    """Parser for OMT (Open Media Toolkit) formatted streams"""
    
    OMT_SIGNATURE_V1 = b'0MFS'
    OMT_SIGNATURE_V2 = b'0MF2'
    OMT_SIGNATURE_V3 = b'0MF3'
    
    def __init__(self, filepath=None):
        self.filepath = filepath
        self.chunks = []
        self.version = 0
        self.raw_header = None
        
    def open_file(self, filepath):
        """Open and parse an OMT file"""
        self.filepath = filepath
        self.chunks = []
        
        try:
            file_size = os.path.getsize(filepath)
            print(f"\n=== Opening OMT File ===")
            print(f"File: {filepath}")
            print(f"File size: {file_size} bytes")
            
            with open(filepath, 'rb') as f:
                file_data = f.read()
            
            # Search for the filename
            filename_search = b"Electro1b powerplant"
            pos = file_data.find(filename_search)
            if pos >= 0:
                print(f"\nFound filename at offset {pos} (0x{pos:08x})")
                # Show 16 bytes before and after
                start = max(0, pos - 16)
                end = min(len(file_data), pos + len(filename_search) + 16)
                context = file_data[start:end]
                print(f"Context (hex): {context.hex()}")
                
                # Check what's immediately before
                if pos >= 4:
                    before_4 = file_data[pos-4:pos]
                    print(f"4 bytes before filename: {before_4.hex()}")
                    print(f"  As BE uint: {struct.unpack('>I', before_4)[0]}")
                    print(f"  As LE uint: {struct.unpack('<I', before_4)[0]}")
            
            with open(filepath, 'rb') as f:
                f.seek(0)
                signature = f.read(4)
                print(f"\nSignature: {signature.hex()}")
                
                if signature == self.OMT_SIGNATURE_V2:
                    self.version = 2
                elif signature == self.OMT_SIGNATURE_V3:
                    self.version = 3
                else:
                    self.version = 1
                
                # Read header offset
                next_bytes = f.read(4)
                le_val = struct.unpack('<I', next_bytes)[0]
                be_val = struct.unpack('>I', next_bytes)[0]
                
                print(f"Header offset candidates: LE={le_val}, BE={be_val}")
                
                if 0 < le_val < file_size:
                    header_offset = le_val
                    endian = '<'
                    print(f"Using LE offset: {header_offset}")
                elif 0 < be_val < file_size:
                    header_offset = be_val
                    endian = '>'
                    print(f"Using BE offset: {header_offset}")
                else:
                    print("No valid offset found")
                    return False
                
                f.seek(header_offset)
                self._parse_header(f, endian)
                print(f"\nSuccessfully parsed {len(self.chunks)} chunks")
                
            return True
        except Exception as e:
            import traceback
            print(f"\nERROR: {e}")
            traceback.print_exc()
            return False
    
    def _scan_for_structure(self, data):
        """Scan file for recognizable chunk patterns"""
        print("Looking for chunk type signatures (4 ASCII characters)...")
        
        # Look for common 4-char codes
        common_types = [b'0HDR', b'3DBS', b'SND ', b'IMAG', b'TXTR']
        
        for chunk_type in common_types:
            pos = data.find(chunk_type)
            if pos >= 0:
                print(f"  Found '{chunk_type.decode('ascii', errors='ignore')}' at offset {pos} (0x{pos:08x})")
    
    def _parse_header(self, f, endian='>'):
        """Parse the OMT header structure"""
        fmt = endian + 'I'
        fmt_i = endian + 'i'
        
        print(f"Using endianness: {'big-endian' if endian == '>' else 'little-endian'}")
        
        num_chunk_types_bytes = f.read(4)
        num_chunk_types = struct.unpack(fmt, num_chunk_types_bytes)[0]
        print(f"Number of chunk types: {num_chunk_types}")
        
        if num_chunk_types > 100:  # Sanity check
            print("WARNING: Unreasonable number of chunk types!")
            return
        
        for i in range(num_chunk_types):
            chunk_type = f.read(4)
            print(f"\nChunk type {i+1}: {chunk_type.hex()}")
            
            num_chunks_bytes = f.read(4)
            num_chunks = struct.unpack(fmt, num_chunks_bytes)[0]
            print(f"  Number of chunks: {num_chunks}")
            
            if num_chunks > 10000:  # Sanity check
                print(f"WARNING: Unreasonable number of chunks: {num_chunks}")
                break
            
            for j in range(num_chunks):
                chunk_info = {}
                
                chunk_info['id'] = struct.unpack(fmt_i, f.read(4))[0]
                chunk_info['offset'] = struct.unpack(fmt, f.read(4))[0]
                chunk_info['size'] = struct.unpack(fmt, f.read(4))[0]
                
                print(f"    Chunk {j+1}: ID={chunk_info['id']}, Offset={chunk_info['offset']}, Size={chunk_info['size']}")
                
                # Validate chunk data makes sense
                if chunk_info['offset'] == 0 or chunk_info['size'] == 0:
                    print(f"      WARNING: Invalid chunk offset/size, skipping")
                    continue
                
                # For version 2+, read name as BYTE length + string
                if self.version >= 2:
                    # Read name length as SINGLE BYTE (not 4 bytes!)
                    name_len_byte = f.read(1)
                    if len(name_len_byte) > 0:
                        name_len = name_len_byte[0]  # Get the byte value
                        
                        print(f"      Name length (byte): {name_len}")
                        
                        # Validate name length (max 256 chars should be reasonable)
                        if name_len > 0 and name_len < 256:
                            name_bytes = f.read(name_len)
                            # Try to decode, but skip if it's garbage
                            try:
                                decoded_name = name_bytes.decode('utf-8', errors='strict').strip()
                                # Check if name contains mostly printable characters
                                if all(32 <= ord(c) < 127 or c in '\t\n\r' for c in decoded_name):
                                    chunk_info['name'] = decoded_name
                                    print(f"      Name: '{chunk_info['name']}'")
                                else:
                                    chunk_info['name'] = ""
                                    print(f"      Name contains non-printable chars, skipping")
                            except UnicodeDecodeError:
                                chunk_info['name'] = ""
                                print(f"      Name decode failed, skipping")
                        else:
                            chunk_info['name'] = ""
                            if name_len > 0:
                                print(f"      Name length unreasonable ({name_len}), skipping")
                    else:
                        chunk_info['name'] = ""
                else:
                    chunk_info['name'] = ""
                
                if self.version >= 3:
                    chunk_info['compression'] = struct.unpack(fmt, f.read(4))[0]
                else:
                    chunk_info['compression'] = 0
                
                try:
                    chunk_info['type'] = chunk_type.decode('ascii')
                except:
                    chunk_info['type'] = chunk_type.hex()
                
                self.chunks.append(chunk_info)
        
        # Try to read free blocks
        try:
            num_free_blocks = struct.unpack(fmt, f.read(4))[0]
            print(f"\nNumber of free blocks: {num_free_blocks}")
        except:
            pass

    def get_chunk_data(self, chunk_index):
        """Extract data from a specific chunk"""
        if chunk_index < 0 or chunk_index >= len(self.chunks):
            return None
        
        chunk = self.chunks[chunk_index]
        
        try:
            with open(self.filepath, 'rb') as f:
                f.seek(chunk['offset'])
                data = f.read(chunk['size'])
                
                # Handle compression
                if chunk['compression'] > 0 and len(data) > 4:
                    uncompressed_size = struct.unpack('>I', data[:4])[0]
                    compressed_data = data[4:]
                    data = zlib.decompress(compressed_data)
                
                return data
        except Exception as e:
            print(f"Error reading chunk data: {e}")
            return None

class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("OMT File Viewer")
        self.omt_parser = None
        self.current_file = None

        # Central layout with left file explorer and right content area
        main_widget = QWidget(self)
        main_layout = QHBoxLayout(main_widget)
        main_layout.setContentsMargins(0, 0, 0, 0)

        self.explorer_container = QWidget(self)
        explorer_layout = QVBoxLayout(self.explorer_container)
        explorer_layout.setContentsMargins(8, 8, 8, 8)
        explorer_label = QLabel("File Explorer", self)
        explorer_label.setAlignment(Qt.AlignmentFlag.AlignLeft | Qt.AlignmentFlag.AlignVCenter)
        explorer_layout.addWidget(explorer_label)

        self.file_tree = QTreeWidget(self)
        self.file_tree.setHeaderLabels(["Name", "Type", "ID"])
        self.file_tree.setColumnWidth(0, 180)
        self.file_tree.setColumnWidth(1, 90)
        self.file_tree.itemClicked.connect(self.update_preview)
        explorer_layout.addWidget(self.file_tree)

        self.explorer_container.setMinimumWidth(280)
        main_layout.addWidget(self.explorer_container)

        # Right side: Preview area
        preview_container = QWidget(self)
        preview_layout = QVBoxLayout(preview_container)
        preview_layout.setContentsMargins(8, 8, 8, 8)
        
        preview_header = QLabel("Preview", self)
        preview_header.setAlignment(Qt.AlignmentFlag.AlignLeft | Qt.AlignmentFlag.AlignVCenter)
        preview_layout.addWidget(preview_header)

        self.preview_stack = QStackedWidget(self)
        
        # Default preview
        self.default_preview = QLabel("Open an OMT file to view its contents")
        self.default_preview.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.preview_stack.addWidget(self.default_preview)

        # Image preview
        self.image_preview = QLabel()
        self.image_preview.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.image_preview.setScaledContents(False)
        self.preview_stack.addWidget(self.image_preview)

        # 3D model preview
        self.model_preview = QLabel("3D Model Viewer\n(To be implemented)")
        self.model_preview.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.model_preview.setStyleSheet("background-color: #2b2b2b; color: #ffffff;")
        self.preview_stack.addWidget(self.model_preview)

        # Audio player preview
        audio_widget = QWidget()
        audio_layout = QVBoxLayout(audio_widget)
        audio_layout.setAlignment(Qt.AlignmentFlag.AlignCenter)
        
        self.audio_label = QLabel("Audio Player")
        self.audio_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        audio_layout.addWidget(self.audio_label)
        
        self.play_button = QPushButton("Play")
        self.play_button.clicked.connect(self.toggle_playback)
        audio_layout.addWidget(self.play_button)
        
        self.audio_slider = QSlider(Qt.Orientation.Horizontal)
        audio_layout.addWidget(self.audio_slider)
        
        self.media_player = QMediaPlayer()
        self.audio_output = QAudioOutput()
        self.media_player.setAudioOutput(self.audio_output)
        
        self.preview_stack.addWidget(audio_widget)

        preview_layout.addWidget(self.preview_stack)
        main_layout.addWidget(preview_container, 1)

        self.setCentralWidget(main_widget)

        toolbar = QToolBar("My main toolbar")
        toolbar.setIconSize(QSize(16, 16))
        self.addToolBar(toolbar)

        button_action = QAction("Explorer", self)
        button_action.setStatusTip("Toggle file explorer")
        button_action.triggered.connect(self.toggle_file_explorer)
        button_action.setCheckable(True)
        button_action.setChecked(True)
        toolbar.addAction(button_action)

        toolbar.addSeparator()

        # Toolbar actions
        open_action = QAction("Open", self)
        open_action.setStatusTip("Open an OMT file")
        open_action.triggered.connect(self.open_file)
        toolbar.addAction(open_action)

        save_action = QAction("Save", self)
        save_action.setStatusTip("Save current file")
        save_action.triggered.connect(self.save_file)
        toolbar.addAction(save_action)

        new_action = QAction("New", self)
        new_action.setStatusTip("Create new OMT file")
        new_action.triggered.connect(self.new_file)
        toolbar.addAction(new_action)

        import_action = QAction("Import", self)
        import_action.setStatusTip("Import resources")
        import_action.triggered.connect(self.import_file)
        toolbar.addAction(import_action)

        export_action = QAction("Export", self)
        export_action.setStatusTip("Export project")
        export_action.triggered.connect(self.export_file)
        toolbar.addAction(export_action)

        toolbar.addSeparator()

        self.setStatusBar(QStatusBar(self))
        self.setFixedSize(QSize(1000, 600))

    def toggle_file_explorer(self, checked):
        """Toggle file explorer visibility"""
        self.explorer_container.setVisible(checked)

    def update_preview(self, item, column):
        """Update preview based on selected file type"""
        chunk_idx = item.data(0, Qt.UserRole)
        if chunk_idx is None:
            return
        
        chunk = self.omt_parser.chunks[chunk_idx]
        file_type = chunk.get('type', '').lower()
        file_name = item.text(0)
        
        # Try to load and preview based on chunk type
        if file_type == 'Canv':
            # Try to decode as canvas (texture)
            chunk_data = self.omt_parser.get_chunk_data(chunk_idx)
            if chunk_data:
                img = OMTCanvasDecoder.decode_canvas(chunk_data)
                if img:
                    # Convert PIL image to QPixmap
                    buf = io.BytesIO()
                    img.save(buf, format='PNG')
                    from PySide6.QtGui import QImage, QPixmap
                    qimg = QImage.fromData(buf.getvalue(), 'PNG')
                    pix = QPixmap.fromImage(qimg)
                    self.image_preview.setPixmap(pix.scaled(400, 400, Qt.AspectRatioMode.KeepAspectRatio, Qt.TransformationMode.SmoothTransformation))
                    self.preview_stack.setCurrentWidget(self.image_preview)
                    return
        
        elif file_type == '3dsp':
            # Try to decode as 3D mesh
            chunk_data = self.omt_parser.get_chunk_data(chunk_idx)
            if chunk_data:
                mesh_info = OMT3DMeshDecoder.decode_3dsh(chunk_data)
                if mesh_info:
                    msg = f"3D Model: {file_name}\n"
                    msg += f"Version: {mesh_info['version']}\n"
                    msg += f"Vertices: {len(mesh_info['vertices'])}\n"
                    msg += f"Polygons: {mesh_info['polygon_count']}"
                    self.model_preview.setText(msg)
                    self.preview_stack.setCurrentWidget(self.model_preview)
                    return
        
        elif file_type == 'wave' or file_type == 'snd ':
            self.audio_label.setText(f"Audio: {file_name}")
            self.preview_stack.setCurrentWidget(self.preview_stack.widget(3))
            return
        
        # Default fallback
        self.default_preview.setText(f"Preview for {file_name}\n(Type: {file_type})")
        self.preview_stack.setCurrentWidget(self.default_preview)

    def toggle_playback(self):
        """Toggle audio playback"""
        if self.media_player.playbackState() == QMediaPlayer.PlaybackState.PlayingState:
            self.media_player.pause()
            self.play_button.setText("Play")
        else:
            self.media_player.play()
            self.play_button.setText("Pause")

    def open_file(self):
        """Open an OMT file"""
        file_path, _ = QFileDialog.getOpenFileName(
            self,
            "Open OMT File",
            "",
            "OMT Files (*.omt);;All Files (*.*)"
        )
        
        if file_path:
            print(f"Opening file: {file_path}")
            self.omt_parser = OMTFormatParser()
            if self.omt_parser.open_file(file_path):
                self.current_file = file_path
                self.load_chunks_to_tree()
                self.statusBar().showMessage(f"Opened: {os.path.basename(file_path)} (Version {self.omt_parser.version})")
            else:
                error_msg = "Failed to open OMT file. Check console for details."
                QMessageBox.critical(self, "Error", error_msg)
                print(error_msg)

    def load_chunks_to_tree(self):
        """Load chunks from OMT file into tree widget"""
        self.file_tree.clear()
        
        if not self.omt_parser:
            return
        
        for idx, chunk in enumerate(self.omt_parser.chunks):
            # Try to get a proper filename
            name = chunk.get('name', '')
            
            # If no name, generate one from chunk type and ID
            if not name or name.strip() == '':
                chunk_type = chunk.get('type', 'UNKN')
                chunk_id = chunk.get('id', idx)
                name = f"{chunk_type}_{chunk_id}"
            
            chunk_type = chunk.get('type', 'Unknown')
            chunk_id = str(chunk.get('id', idx))
            
            print(f"Adding to tree: Name='{name}', Type='{chunk_type}', ID={chunk_id}")
            
            item = QTreeWidgetItem(self.file_tree, [name, chunk_type, chunk_id])
            # Store chunk index for later retrieval
            item.setData(0, Qt.UserRole, idx)

    def save_file(self):
        """Save current OMT file"""
        if not self.current_file:
            QMessageBox.information(self, "Save", "No file currently open")
            return
        print("Save action triggered")

    def new_file(self):
        """Create new OMT file"""
        file_path, _ = QFileDialog.getSaveFileName(
            self,
            "Create New OMT File",
            "",
            "OMT Files (*.omt);;All Files (*.*)"
        )
        
        if file_path:
            # Create new empty OMT file structure
            self.omt_parser = OMTFormatParser()
            self.current_file = file_path
            self.file_tree.clear()
            self.statusBar().showMessage(f"Created: {os.path.basename(file_path)}")
            print(f"New file created: {file_path}")

    def import_file(self):
        """Import resources into OMT file"""
        if not self.current_file:
            QMessageBox.information(self, "Import", "Please open or create an OMT file first")
            return
        print("Import action triggered")

    def export_file(self):
        """Export project"""
        if not self.current_file:
            QMessageBox.information(self, "Export", "No file currently open")
            return
        print("Export action triggered")


if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = MainWindow()
    window.show()
    sys.exit(app.exec())