#!/usr/bin/env python3
"""Generate HTML texture gallery from PNG assets."""

import os
from pathlib import Path

ASSETS_PNG = Path.home() / 'jn-engine' / 'assets' / 'png'
DOCS_DIR = Path.home() / 'jn-engine' / 'docs'
GALLERY_HTML = DOCS_DIR / 'texture-gallery.html'

def generate_gallery():
    """Generate HTML gallery from PNG files."""
    pngs = sorted([f for f in ASSETS_PNG.iterdir() if f.suffix.lower() == '.png'])

    html = '''<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>JNBG Texture Gallery</title>
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
        .info {
            color: #999;
            margin-bottom: 2rem;
            font-size: 0.9rem;
        }
        .gallery {
            display: grid;
            grid-template-columns: repeat(auto-fill, minmax(150px, 1fr));
            gap: 1.5rem;
        }
        .texture-item {
            display: flex;
            flex-direction: column;
            align-items: center;
            cursor: pointer;
            transition: transform 0.2s;
        }
        .texture-item:hover {
            transform: scale(1.05);
        }
        .texture-thumb {
            width: 140px;
            height: 140px;
            background: #333;
            border: 1px solid #555;
            border-radius: 4px;
            display: flex;
            align-items: center;
            justify-content: center;
            overflow: hidden;
        }
        .texture-thumb img {
            max-width: 100%;
            max-height: 100%;
            width: auto;
            height: auto;
        }
        .texture-name {
            margin-top: 0.5rem;
            font-size: 0.85rem;
            text-align: center;
            word-break: break-word;
            width: 140px;
            color: #b0b0b0;
        }
        a {
            color: #4a9eff;
            text-decoration: none;
        }
        a:hover {
            text-decoration: underline;
        }
    </style>
</head>
<body>
    <h1>JNBG Texture Gallery</h1>
    <div class="info">
        <p>''' + str(len(pngs)) + ''' textures · See <a href="texture-catalog.md">texture-catalog.md</a> for annotations</p>
    </div>
    <div class="gallery">
'''

    for png in pngs:
        rel_path = f'../assets/png/{png.name}'
        html += f'''        <div class="texture-item">
            <div class="texture-thumb">
                <img src="{rel_path}" alt="{png.stem}">
            </div>
            <div class="texture-name">{png.stem}</div>
        </div>
'''

    html += '''    </div>
</body>
</html>
'''

    GALLERY_HTML.write_text(html)
    print(f'✓ Generated {GALLERY_HTML} ({len(pngs)} textures)')

if __name__ == '__main__':
    generate_gallery()
