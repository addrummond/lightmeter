import png
import os
import sys
import re

#
# We compress 12x12 digits by splitting them into 4x4 blocks and
# hoping that the number of unique 4x4 patterns will be small. (The
# blocky design of the digits helps with this.)
#
# 8x8 digits are not currently compressed.
#
# Following pypng, we treat images as lists of lists (lists of rows).
#

BLOCK_SIZE = 4

def get_unique_blocks(image, offset, width=12, height=12, blocks=None):
    assert offset <= 2

    if blocks is None:
        blocks = [ ]

    block_grid = [ ]
    num_blocks = 0
    for y in range(0, height, BLOCK_SIZE):
        block_grid.append([ ])
        for x in range(0, width, BLOCK_SIZE):
            block = [[0 for _ in range(0, BLOCK_SIZE)] for __ in range(0, BLOCK_SIZE)]
            by = 0
            for yy in range(y, y+BLOCK_SIZE):
                if yy >= height:
                    yy -= height
                bx = 0
                for xx in range(x, x+BLOCK_SIZE):
                    if xx >= width:
                        xx -= width
#                    print("BY %i, BX %i, yy %i, xx %i" % (by,bx, yy,xx))
                    # Note: blocks are column major.
                    block[bx][by] = image[yy][xx]
                    bx += 1
                by += 1
#            print("\n\n")
#            block = colmajor_block(block)
            # Invert block vertically.
            for x in block:
                x.reverse()
            index = None
            try:
                index = ('original', blocks.index(block))
            except ValueError:
                pass
            if index is None:
                blocks.append(block)
                index = ('original', len(blocks)-1)
            block_grid[-1].append(index)
            num_blocks += 1
    # Invert block grid vertically.
    block_grid.reverse()

    return blocks, block_grid, num_blocks

def rgba_to_mono(pixels, width):
    pixels2 = [[0 if y else 1 for y in x] for x in pixels]
    if len(pixels[0]) == width*3:
        return [[row[i] for i in range(0, len(row), 3)] for row in pixels2] # RGB -> monochrome
    if len(pixels[0]) == width*4:
        return [[row[i] for i in range(0, len(row), 4)] for row in pixels2] # RGBA -> monochrome

def get_12px_chars():
     blocks = [ ]
     bitmap_count = 0
     name_to_block_grid = { }
     max_blocks_per_char = 0
     for name in os.listdir("./"):
         if name.startswith("12px_") and name.endswith(".png"):
             bitmap_count += 1
             r = png.Reader(file=open(name))
             img = r.read()
             width = img[0]
             height = img[1]
             pixels = rgba_to_mono(list(img[2]), 12)
             # Drop first and last two columns for 12x8.
             pixels = [x[2:-2] for x in pixels]
#             for row in pixels:
#                 for p in row:
#                     sys.stdout.write("%i " % p)
#                 print()
#             print(pixels)
             blocks, block_grid, num_blocks = get_unique_blocks(pixels, 0, width-4, height, blocks)
             name_to_block_grid[name] = block_grid
             max_blocks_per_char = max(max_blocks_per_char, num_blocks)

     return blocks, bitmap_count, name_to_block_grid, max_blocks_per_char

def get_8px_chars():
    char_px_grids = { }
    for name in os.listdir("./"):
        if name.startswith("8px_") and name.endswith(".png"):
            r = png.Reader(file=open(name))
            img = r.read()
            width = img[0]
            height = img[1]
            pixels = rgba_to_mono(list(img[2]), 8)
            # Get in col major format vertically inverted.
            pixels = [[pixels[j][i] for j in reversed(range(len(pixels)))] for i in range(len(pixels[0]))]
            # Drop first and last columns for 8x6.
            pixels = pixels[1:-1]
            char_px_grids[name] = pixels
    return char_px_grids

def get_stats():
     blocks_array, bitmap_count, name_to_block_grid, max_blocks_per_char = get_12px_chars()

     for blk in blocks_array:
         for row in blk:
             print(' '.join(map(str, row)))
         print("\n============\n")

     blockcount = len(blocks_array)
     uncompressed = bitmap_count*12*8/8
     compressed = blockcount*BLOCK_SIZE*BLOCK_SIZE/8 + bitmap_count*(12/BLOCK_SIZE)*(8/BLOCK_SIZE)

     print("There are %i blocks" % blockcount)
     print("Maximum blocks per char: %i" % max_blocks_per_char)
     print("Size uncompressed %i" % uncompressed)
     print("Size compressed %i" % compressed)

def print_test_chars():
    blocks_array, bitmap_count, name_to_block_grid, max_blocks_per_char = get_12px_chars()

    for name, grid in name_to_block_grid.iteritems():
        print("Drawing %s:\n" % name)

#
#        [prints individual blocks first]
#
#        for gg in grid:
#            for b in gg:
#                bl = blocks_array[b]
#                for y in range(BLOCK_SIZE):
#                    for x in range(BLOCK_SIZE):
#                        v = bl[x][y]
#                        sys.stdout.write("%i " % v)
#                    sys.stdout.write("\n")
#                sys.stdout.write("\n\n")

        for line in range(11, -1, -1):
            for col in range(8):
                flp, index = grid[line/BLOCK_SIZE][col/BLOCK_SIZE]
                block = blocks_array[index]
                if flp == 'flipv':
                    block = list(reversed(block))
                elif flp == 'fliph':
                    block = [list(reversed(aa)) for aa in block]
                sys.stdout.write("%s " % block[col % BLOCK_SIZE][line % BLOCK_SIZE])
            sys.stdout.write("\n")
        sys.stdout.write("\n\n")

def output_bit(counter, f, b):
    if counter[0] % 8 == 0:
        if counter[0] != 0:
            f.write(',')
        f.write("0b")
    f.write(str(b))
    counter[0] += 1

def output_tables():
    dotc = open("bitmaps.c", "w")
    doth = open("bitmaps.h", "w")

    #
    # PREAMBLE
    #
    doth.write("#ifndef BITMAPS_H\n#define BITMAPS_H\n\n")
    doth.write("#include <stdint.h>\n\n")
    dotc.write("#include <stdint.h>\n")

    doth.write("#define CHAR_WIDTH_8PX 6\n")
    doth.write('#define CHAR_WIDTH_12PX 8\n')
    doth.write("#define CHAR_OFFSET_8PX(n) (((n) << 2) + ((n) << 1)) // I.e. n*6\n")
    doth.write("#define CHAR_OFFSET_12PX(n) (((n) << 2) + ((n) << 1)) // I.e. n*6\n")
    doth.write("#define CHAR_OFFSET_FROM_CODE_8PX(n) CHAR_OFFSET_8PX(n-1)\n")
    doth.write("#define CHAR_OFFSET_FROM_CODE_12PX(n) CHAR_OFFSET_12PX(n-1)\n\n")

    #
    # TABLES FOR 12PX CHARS
    #

    def output_block_comment(block, index, f):
        f.write("// index: %i\n" % index)
        for row in range(BLOCK_SIZE):
            f.write("//    ")
            for col in range(BLOCK_SIZE):
                f.write("%s " % block[col][row])
            f.write("\n")

    doth.write("extern const uint8_t CHAR_BLOCKS_12PX[];\n")
    dotc.write("const uint8_t CHAR_BLOCKS_12PX[] = {\n    ")
    blocks_array, bitmap_count, name_to_block_grid, max_blocks_per_char = get_12px_chars()
    barray_index = 0
    bit_counter = [0]
    for b in blocks_array:
        assert BLOCK_SIZE == 4
        output_block_comment(b, barray_index * 2, dotc)
        for r in b:
            for bit in r:
                output_bit(bit_counter, dotc, bit)
#            dotc.write('    ' + ', '.join(map(str, r)) + ',\n')
        dotc.write("\n    ")
        barray_index += 1
    dotc.write("\n};\n")

    doth.write('extern const uint8_t CHAR_12PX_GRIDS[];\n')
    dotc.write('const uint8_t CHAR_12PX_GRIDS[] = {\n')
    i = 0
    code = 1
    ordered_keys = name_to_block_grid.keys()
    ordered_keys.sort()
    for name in ordered_keys:
        grid = name_to_block_grid[name]
        m = re.match(r"^12px_([^.]+)\.png$", name)
        assert m
        cname = 'CHAR_12PX_' + m.group(1).upper()
        doth.write('#define ' + cname + ' (CHAR_12PX_GRIDS + ' + str(i) + ')\n')
        doth.write('#define ' + cname + '_O ' + str(i) + '\n')
        doth.write('#define ' + cname + '_CODE ' + str(code) + '\n')
        dotc.write("// " + cname + "\n")
        for row in grid:
            assert BLOCK_SIZE == 4
            def genindex(index):
                assert index[1] <= 255 and index[0] == 'original'
                return index[1]
            dotc.write('    ' + ', '.join(map(str, (map(genindex, row)))) + ',\n')
        dotc.write('\n')
        i += (12/BLOCK_SIZE)*(8/BLOCK_SIZE)
        code += 1
    dotc.write('};\n')

    doth.write('#define CHAR_12PX_BLOCK_SIZE ' + str(BLOCK_SIZE))

    #
    # TABLES FOR 8PX CHARS.
    #
    doth.write("\n\nextern const uint8_t CHAR_PIXELS_8PX[];\n")
    dotc.write("\n\nconst uint8_t CHAR_PIXELS_8PX[] = {\n")
    pgrids = get_8px_chars()
    ordered_keys = pgrids.keys()
    ordered_keys.sort()
    bit_counter = [0]
    i = 0
    for name in ordered_keys:
        g = pgrids[name]
        m = re.match(r"^8px_([^.]+)\.png", name)
        assert m
        cname = 'CHAR_8PX_' + m.group(1).upper()
        doth.write("#define " + cname + ' (CHAR_PIXELS_8PX + ' + str(i*6) + ')\n')
        doth.write("#define " + cname + '_O ' + str(i*6) + '\n')
        dotc.write("    ")
        for c in g:
            for px in c:
                output_bit(bit_counter, dotc, px)
            dotc.write("\n    ")
        dotc.write("\n\n")

        i += 1
    dotc.write("};\n")

    #
    # POSTAMBLE
    #

    doth.write("\n#endif\n")

    dotc.close()
    doth.close()

if __name__ == '__main__':
    assert len(sys.argv) > 1

    if sys.argv[1] == 'stats':
        get_stats()
    elif sys.argv[1] == 'output':
        output_tables()
    elif sys.argv[1] == 'testchars':
        print_test_chars()
    else:
        assert False
