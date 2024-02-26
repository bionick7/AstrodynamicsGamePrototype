from PIL import Image
import PIL.ImageOps
import skfmm
import numpy as np
import regex as re

def main(base_font, new_font, base_size, downsampled_size):
    with open(base_font) as f:
        lines = f.readlines()

    counter = 0

    base_retangles = []

    for i, l in enumerate(lines[4:]):
        re_search = re.search(r"x=(\d+)\s+y=(\d+)\s+width=(\d+)\s+height=(\d+)", l)
        if re_search is None:
            raise LookupError(f"Couldn't find pattern 'x=(\\d+)\\s+y=(\\d+)\\s+width=(\\d+)\\s+height=(\\d+)' in line {i+5}")
        x = int(re_search.group(1))
        y = int(re_search.group(2))
        w = int(re_search.group(3))
        h = int(re_search.group(4))
        base_retangles.append([x, x+w, y, y+h])
        x = (x * downsampled_size) // base_size
        y = (y * downsampled_size) // base_size
        w = (w * downsampled_size) // base_size
        h = (h * downsampled_size) // base_size

        group_start, group_end = re_search.span()
        group_end += 4 - len(str(h))
        l = l[:group_start] + f"x={x:<4}  y={y:<4}  width={w:<4}  height={h:<4}" + l[group_end:]
        l = re.sub(r"xadvance=.{4}", f"xadvance={w:<4}", l)
        lines[i+4] = l

    # putting greek delta as alias of math delta
    line_delta = [x for x in lines if x.startswith("char id=8710")][0]
    lines.append(line_delta.replace("8710", "916 "))
    base_retangles.append([0,0,1,1])  # Do nothing here
    
    counter += 1

    char_id = 1024-256
    for y in range(16, 32):
        for x in range(0, 32):
            if x < 320:
                base_retangles.append([x*base_size, (x+1)*base_size, y*base_size, (y+1)*base_size])
                lines.append(
                    f"char id={char_id:<3} x={x*downsampled_size:<3}   "
                    f"y={y*downsampled_size:<3}   width={downsampled_size:<3}   "
                    f"height={downsampled_size:<3}   xoffset=0     yoffset=0     "
                    f"xadvance={downsampled_size:<3}    page=0  chnl=15\n"
                )
            char_id += 1
            counter += 1

    new_font_sdf = new_font.replace(".fnt", "_sdf.fnt")
    new_img_file = new_font.replace(".fnt", "_0.png")
    new_img_file_sdf = new_font_sdf.replace(".fnt", "_0.png")
    old_img_file = lines[2][16:-2]

    for i, l in enumerate(lines[:4]):
        args = l[:-1].split(" ")
        for j in range(1, len(args)):
            if "=" not in args[j]: continue
            k, v = args[j].split("=")
            if k == "size": v = str(downsampled_size)
            if k == "lineHeight": v = str(downsampled_size)
            if k == "scaleW": v = str(downsampled_size * 32)
            if k == "scaleH": v = str(downsampled_size * 32)
            #if k == "base": 
            #    og_base = int(v)
            #    v = str((og_base * downsampled_size) // base_size)
            if k == "file": v = f"\"{new_img_file}\""
            if k == "count": 
                og_count = int(v)
                v = str(og_count + counter)

            args[j] = k + "=" + v
        lines[i] = " ".join(args) + "\n"

    #print("".join(lines))
    with open(new_font, "wt") as f:
        f.write("".join(lines))
    with open(new_font_sdf, "wt") as f:
        lines[2] = re.sub(r"file=\".*\"", f"file=\"{new_img_file_sdf}\"", lines[2])
        f.write("".join(lines))

    # Generate sdfs
    icons_w, icons_h = 16*base_size, 16*base_size
    im_icons = Image.open("../icons/font_icons.png").resize((icons_w, icons_h)).convert("L")
    im_icons = PIL.ImageOps.invert(im_icons)
    im_font = Image.open(old_img_file)
    font_w, font_h = im_font.size

    offset = (0, font_h - icons_h)
    im_font.paste(im_icons, offset)
    #im_font.show()

    phi = np.array(im_font) / 255
    sdf_final = np.ones(phi.shape)

    """
    phi = np.where(phi, 0, -1) + 0.5
    sdf_out = skfmm.distance(-phi, dx = 0.03)
    sdf_in = skfmm.distance(phi, dx = 0.03)
    sdf_final = (1 + sdf_out - sdf_in) * 0.5
    """

    for i, l in enumerate(lines[4:]):
        x1, x2, y1, y2 = base_retangles[i]
        phi_local = phi[y1:y2,x1:x2]
        if np.count_nonzero(phi_local) == 0:
            continue
        phi_local = np.where(phi_local, 0, -1) + 0.5
        try:
            sdf_out = skfmm.distance(-phi_local, dx = 1 / base_size)
            sdf_in = skfmm.distance(phi_local, dx = 1 / base_size)
        except ValueError as e:
            print(f"Error in line {i+5}: {l}")
            raise
        else:
            sdf_final[y1:y2,x1:x2] = (1 + sdf_out - sdf_in) * 0.5
            
    im_sdf = Image.fromarray(sdf_final * 255).convert(im_font.mode)
    im_sdf.show()
    im_sdf = im_sdf.resize((downsampled_size*32, downsampled_size*32))
    im_sdf.save(new_img_file_sdf)
    im_font = im_font.resize((downsampled_size*32, downsampled_size*32))
    im_font.save(new_img_file)

if __name__ == "__main__":
    main("space_mono.fnt", "space_mono_small.fnt", 128, 40)