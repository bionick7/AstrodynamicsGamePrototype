from PIL import Image
import PIL.ImageOps

def modify_text():
    with open("space_mono_small.fnt") as f:
        lines = f.readlines()

    counter = 0

    # putting greek delta as alias of math delta
    line_delta = [x for x in lines if x.startswith("char id=8710")][0]
    lines.append(line_delta.replace("8710", "916 "))
    
    counter += 1

    char_id = 1024-256
    for y in range(320, 640, 20):
        for x in range(0, 640, 20):
            if x < 320:
                lines.append(f"char id={char_id:<3} x={x:<3}   y={y:<3}   width=20    height=20    xoffset=0     yoffset=0     xadvance=20    page=0  chnl=15\n")
            char_id += 1
            counter += 1

    og_count = int(lines[3][12:-1])
    lines[2] = "page id=0 file=\"space_mono_small_0_ex.png\"\n"
    lines[3] = f"chars count={og_count + counter}\n"
    #print("".join(lines))
    with open("space_mono_small_ex.fnt", "wt") as f:
        f.write("".join(lines))


def modify_image():
    icons_w, icons_h = 320, 320
    im_icons = Image.open("../icons/font_icons.png").resize((icons_w, icons_h))
    im_icons = PIL.ImageOps.invert(im_icons)
    im_font = Image.open("space_mono_small_0.png")
    font_w, font_h = im_font.size

    offset = (0, font_h - icons_h)
    im_font.paste(im_icons, offset)
    im_font.save("space_mono_small_0_ex.png")

def main():
    modify_image()
    modify_text()

if __name__ == "__main__":
    main()