from PIL import Image, ImageDraw, ImageFilter
import os

S = 1024
im = Image.new('RGBA', (S, S), (0, 0, 0, 0))
d = ImageDraw.Draw(im)
# Glow layer
glow = Image.new('RGBA', (S, S), (0, 0, 0, 0))
gd = ImageDraw.Draw(glow)
pts = [(190, 850), (345, 150), (700, 890), (855, 155)]
gd.line(pts, fill=(99, 87, 232, 180), width=180, joint='curve')
for p in [pts[0], pts[-1]]:
    gd.ellipse([p[0]-90, p[1]-90, p[0]+90, p[1]+90], fill=(99, 87, 232, 180))
glow = glow.filter(ImageFilter.GaussianBlur(65))
im.alpha_composite(glow)

# Outer stroke
d.line(pts, fill=(65, 54, 190, 255), width=150, joint='curve')
for p in [pts[0], pts[-1]]:
    d.ellipse([p[0]-75, p[1]-75, p[0]+75, p[1]+75], fill=(65, 54, 190, 255))
# Main purple ribbon
d.line(pts, fill=(103, 91, 236, 255), width=112, joint='curve')
for p in [pts[0], pts[-1]]:
    d.ellipse([p[0]-56, p[1]-56, p[0]+56, p[1]+56], fill=(103, 91, 236, 255))
# Inner highlight
d.line(pts, fill=(152, 140, 255, 255), width=46, joint='curve')
for p in [pts[0], pts[-1]]:
    d.ellipse([p[0]-23, p[1]-23, p[0]+23, p[1]+23], fill=(152, 140, 255, 255))

os.makedirs(r'F:\Projects\novoline-rat-launcher\assets', exist_ok=True)
im.save(r'F:\Projects\novoline-rat-launcher\assets\novoline_1024.png')
im.save(r'F:\Projects\novoline-rat-launcher\assets\novoline.ico',
        sizes=[(256, 256), (128, 128), (64, 64), (48, 48), (32, 32), (16, 16)])
for size in (256, 48, 32, 16):
    im.resize((size, size), Image.Resampling.LANCZOS).save(rf'F:\Projects\novoline-rat-launcher\assets\icon_{size}.png')
print('icon written')
