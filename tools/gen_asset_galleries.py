#!/usr/bin/env python3
"""Generate asset galleries for all asset types: textures, parsed, 3D models."""

import os
from pathlib import Path
from collections import defaultdict

ASSETS_DIR = Path.home() / 'jn-engine' / 'assets'
DOCS_DIR = Path.home() / 'jn-engine' / 'docs'

def get_asset_categories():
    """Scan assets directory and categorize all assets. Every OMT container
    extracted under assets/parsed/<name>/<name>_images/ becomes a category
    automatically (see tools/extract_all_omt.py), so the catalog covers the full
    OMT set, not a hand-picked subset."""
    categories = {
        'textures': {'path': ASSETS_DIR / 'png', 'files': []},
    }
    parsed = ASSETS_DIR / 'parsed'
    if parsed.exists():
        for sub in sorted(parsed.iterdir(), key=lambda p: p.name.lower()):
            img_dir = sub / f'{sub.name}_images'
            if img_dir.is_dir():
                categories[sub.name] = {'path': img_dir, 'files': []}

    for cat, info in categories.items():
        if info['path'].exists():
            files = sorted([f for f in info['path'].iterdir() if f.suffix.lower() == '.png'])
            info['files'] = files

    # Scan 3D models
    ase_files = sorted([f for f in (ASSETS_DIR / 'ase').iterdir() if f.suffix.lower() == '.ase']) if (ASSETS_DIR / 'ase').exists() else []
    omt_files = sorted([f for f in (ASSETS_DIR / 'omt').iterdir() if f.suffix.lower() == '.omt']) if (ASSETS_DIR / 'omt').exists() else []

    return categories, ase_files, omt_files

def generate_html_gallery(cat_name, files, rel_path_prefix):
    """Generate HTML gallery for a category of PNG files."""
    html = f'''<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>JNBG Asset Gallery - {cat_name.title()}</title>
    <style>
        * {{ margin: 0; padding: 0; box-sizing: border-box; }}
        body {{
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            background: #1a1a1a;
            color: #e0e0e0;
            padding: 2rem;
        }}
        h1 {{
            margin-bottom: 0.25rem;
            font-size: 2rem;
        }}
        .breadcrumb {{
            color: #666;
            margin-bottom: 1.5rem;
            font-size: 0.9rem;
        }}
        .breadcrumb a {{ color: #4a9eff; text-decoration: none; }}
        .breadcrumb a:hover {{ text-decoration: underline; }}
        .info {{
            color: #999;
            margin-bottom: 2rem;
            font-size: 0.9rem;
        }}
        .gallery {{
            display: grid;
            grid-template-columns: repeat(auto-fill, minmax(150px, 1fr));
            gap: 1.5rem;
        }}
        .asset-item {{
            display: flex;
            flex-direction: column;
            align-items: center;
            transition: transform 0.2s;
        }}
        .asset-item:hover {{
            transform: scale(1.05);
        }}
        .asset-thumb {{
            width: 140px;
            height: 140px;
            background: #333;
            border: 1px solid #555;
            border-radius: 4px;
            display: flex;
            align-items: center;
            justify-content: center;
            overflow: hidden;
        }}
        .asset-thumb img {{
            max-width: 100%;
            max-height: 100%;
            width: auto;
            height: auto;
        }}
        .asset-name {{
            margin-top: 0.5rem;
            font-size: 0.8rem;
            text-align: center;
            word-break: break-word;
            width: 140px;
            color: #888;
        }}
    </style>
</head>
<body>
    <h1>JNBG Asset Gallery</h1>
    <div class="breadcrumb">
        <a href="asset-index.html">← All Assets</a>
    </div>
    <div class="info">
        <p>{cat_name.upper()} · {len(files)} assets · See <a href="asset-catalog.md">asset-catalog.md</a> for annotations</p>
    </div>
    <div class="gallery">
'''

    for f in files:
        rel_path = f'{rel_path_prefix}/{f.name}'
        html += f'''        <div class="asset-item">
            <div class="asset-thumb">
                <img src="{rel_path}" alt="{f.stem}">
            </div>
            <div class="asset-name">{f.stem}</div>
        </div>
'''

    html += '''    </div>
</body>
</html>
'''
    return html

def generate_index_html(categories, ase_count, omt_count):
    """Generate master index page."""
    total_files = sum(len(cat['files']) for cat in categories.values())
    total_assets = total_files + ase_count + omt_count

    html = '''<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>JNBG Asset Gallery - Index</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            background: #1a1a1a;
            color: #e0e0e0;
            padding: 2rem;
        }
        h1 {
            margin-bottom: 0.5rem;
            font-size: 2rem;
        }
        .summary {
            color: #999;
            margin-bottom: 2rem;
            font-size: 0.9rem;
        }
        .grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(280px, 1fr));
            gap: 1.5rem;
            margin-bottom: 3rem;
        }
        .card {
            background: #252525;
            border: 1px solid #444;
            border-radius: 6px;
            padding: 1.5rem;
            transition: border-color 0.2s;
        }
        .card:hover {
            border-color: #666;
        }
        .card-title {
            font-size: 1.2rem;
            margin-bottom: 0.5rem;
            font-weight: 600;
        }
        .card-count {
            color: #4a9eff;
            font-size: 0.95rem;
            margin-bottom: 1rem;
        }
        .card-link {
            display: inline-block;
            color: #4a9eff;
            text-decoration: none;
            margin-top: 0.5rem;
        }
        .card-link:hover {
            text-decoration: underline;
        }
        .section-title {
            font-size: 1.3rem;
            margin-top: 2rem;
            margin-bottom: 1rem;
            border-bottom: 1px solid #444;
            padding-bottom: 0.5rem;
        }
        .section-desc {
            color: #888;
            margin-bottom: 1rem;
            font-size: 0.9rem;
        }
        .catalog-link {
            display: inline-block;
            margin-top: 2rem;
            padding: 0.75rem 1.5rem;
            background: #333;
            border: 1px solid #555;
            border-radius: 4px;
            color: #4a9eff;
            text-decoration: none;
        }
        .catalog-link:hover {
            background: #3a3a3a;
        }
    </style>
</head>
<body>
    <h1>JNBG Asset Gallery</h1>
    <div class="summary">
        <p>''' + str(total_assets) + ''' total assets · ''' + str(total_files) + ''' 2D images + ''' + str(ase_count) + ''' ASE models + ''' + str(omt_count) + ''' OMT meshes</p>
    </div>

    <div class="section-title">2D Assets</div>
    <div class="section-desc">Textures, sprites, icons, effects, and parsed images</div>
    <div class="grid">
'''

    # 2D assets
    for cat, info in sorted(categories.items()):
        count = len(info['files'])
        if count == 0:
            continue
        html += f'''        <div class="card">
            <div class="card-title">{cat.title()}</div>
            <div class="card-count">{count} assets</div>
            <a href="gallery-{cat}.html" class="card-link">View Gallery →</a>
        </div>
'''

    html += '''    </div>

    <div class="section-title">3D Models</div>
    <div class="section-desc">ASE and OMT mesh files</div>
    <div class="grid">
'''

    # 3D models
    if ase_count > 0:
        html += f'''        <div class="card">
            <div class="card-title">ASE Models</div>
            <div class="card-count">{ase_count} files</div>
            <a href="gallery-ase.html" class="card-link">View List →</a>
        </div>
'''

    if omt_count > 0:
        html += f'''        <div class="card">
            <div class="card-title">OMT Meshes</div>
            <div class="card-count">{omt_count} files</div>
            <a href="gallery-omt.html" class="card-link">View List →</a>
        </div>
'''

    html += '''    </div>

    <a href="asset-catalog.md" class="catalog-link">📋 Asset Catalog (Annotations)</a>
</body>
</html>
'''
    return html

def generate_model_list_html(model_type, files):
    """Generate list page for 3D models."""
    html = f'''<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>JNBG {model_type.upper()} Models</title>
    <style>
        * {{ margin: 0; padding: 0; box-sizing: border-box; }}
        body {{
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            background: #1a1a1a;
            color: #e0e0e0;
            padding: 2rem;
            max-width: 1200px;
            margin: 0 auto;
        }}
        h1 {{
            margin-bottom: 0.25rem;
            font-size: 2rem;
        }}
        .breadcrumb {{
            color: #666;
            margin-bottom: 1.5rem;
            font-size: 0.9rem;
        }}
        .breadcrumb a {{ color: #4a9eff; text-decoration: none; }}
        .breadcrumb a:hover {{ text-decoration: underline; }}
        .info {{
            color: #999;
            margin-bottom: 2rem;
            font-size: 0.9rem;
        }}
        table {{
            width: 100%;
            border-collapse: collapse;
            margin-top: 1rem;
        }}
        th {{
            background: #333;
            padding: 0.75rem;
            text-align: left;
            border-bottom: 1px solid #555;
            font-weight: 600;
            font-size: 0.9rem;
        }}
        td {{
            padding: 0.75rem;
            border-bottom: 1px solid #333;
        }}
        tr:hover {{
            background: #252525;
        }}
        .filename {{
            font-family: monospace;
            font-size: 0.85rem;
            color: #4a9eff;
        }}
        .size {{
            color: #999;
            font-size: 0.85rem;
        }}
    </style>
</head>
<body>
    <h1>JNBG {model_type.upper()} Models</h1>
    <div class="breadcrumb">
        <a href="asset-index.html">← All Assets</a>
    </div>
    <div class="info">
        <p>{len(files)} {model_type.upper()} model files · See <a href="asset-catalog.md">asset-catalog.md</a> for annotations</p>
    </div>

    <table>
        <thead>
            <tr>
                <th>Filename</th>
                <th>Size</th>
                <th>Status</th>
            </tr>
        </thead>
        <tbody>
'''

    for f in files:
        size_kb = f.stat().st_size / 1024
        size_str = f"{size_kb:.1f} KB" if size_kb < 1024 else f"{size_kb/1024:.1f} MB"
        html += f'''            <tr>
                <td class="filename">{f.name}</td>
                <td class="size">{size_str}</td>
                <td>⚠️</td>
            </tr>
'''

    html += '''        </tbody>
    </table>
</body>
</html>
'''
    return html

def main():
    """Generate all galleries."""
    categories, ase_files, omt_files = get_asset_categories()

    # Generate index
    index_html = generate_index_html(categories, len(ase_files), len(omt_files))
    (DOCS_DIR / 'asset-index.html').write_text(index_html)
    print(f'✓ Generated asset-index.html')

    # Generate per-category galleries
    for cat, info in categories.items():
        if len(info['files']) == 0:
            continue

        # Compute relative path from docs to asset files
        rel_path = os.path.relpath(info['path'], DOCS_DIR)
        html = generate_html_gallery(cat, info['files'], rel_path)
        (DOCS_DIR / f'gallery-{cat}.html').write_text(html)
        print(f'✓ Generated gallery-{cat}.html ({len(info["files"])} assets)')

    # Generate 3D model lists
    if ase_files:
        html = generate_model_list_html('ase', ase_files)
        (DOCS_DIR / 'gallery-ase.html').write_text(html)
        print(f'✓ Generated gallery-ase.html ({len(ase_files)} models)')

    if omt_files:
        html = generate_model_list_html('omt', omt_files)
        (DOCS_DIR / 'gallery-omt.html').write_text(html)
        print(f'✓ Generated gallery-omt.html ({len(omt_files)} models)')

    print(f'\n✨ All galleries generated. Start at: {DOCS_DIR}/asset-index.html')

if __name__ == '__main__':
    main()
