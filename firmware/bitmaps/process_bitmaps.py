import png
import os
import sys
import re

#
# We compress 12x12 digits by splitting them into 3x3 blocks and
# hoping that the number of unique 3x3 patterns will be small. (The
# blocky design of the digits helps with this.)
#
# Following pypng, we treat images as lists of lists (lists of rows).
#

BLOCK_WIDTH = 4
BLOCK_HEIGHT = 4

def get_unique_blocks(image, offset, width=12, height=12, blocks=None):
    assert offset <= 2

    if blocks is None:
        blocks = [ ]

    block_grid = [ ]
    num_blocks = 0
    for y in xrange(0, height, BLOCK_HEIGHT):
        block_grid.append([ ])
        for x in xrange(0, width, BLOCK_WIDTH):
            block = [[0 for _ in xrange(0, BLOCK_HEIGHT)] for __ in xrange(0, BLOCK_WIDTH)]
            by = 0
            for yy in xrange(y, y+BLOCK_HEIGHT):
                if yy >= height:
                    yy -= height
                bx = 0
                for xx in xrange(x, x+BLOCK_WIDTH):
                    if xx >= width:
                        xx -= width
#                    print "BY %i, BX %i, yy %i, xx %i" % (by,bx, yy,xx)
                    block[by][bx] = image[yy][xx]
                    bx += 1
                by += 1
#            print "\n\n"
            index = None
            try:
                index = blocks.index(block)
            except ValueError:
                blocks.append(block)
                index = len(blocks)-1
            block_grid[-1].append(index)
            num_blocks += 1

    return blocks, block_grid, num_blocks

def get_12px_blocks():
     blocks = [ ]
     bitmap_count = 0
     name_to_block_grid = { }
     max_blocks_per_char = 0
     for name in os.listdir("./"):
         if name.startswith("12px_"):
             bitmap_count += 1
             r = png.Reader(file=open(name))
             img = r.read()
             width = img[0]
             height = img[1]
             pixels = map(lambda y: map(lambda x: x and 1 or 0, y), img[2])
             pixels = [[row[i] for i in xrange(0, len(row), 3)] for row in pixels]
#             for row in pixels:
#                 for p in row:
#                     sys.stdout.write("%i " % p)
#                 print 
#             print pixels
             blocks, block_grid, num_blocks = get_unique_blocks(pixels, 0, width, height, blocks)
             name_to_block_grid[name] = block_grid
             max_blocks_per_char = max(max_blocks_per_char, num_blocks)

     return blocks, bitmap_count, name_to_block_grid, max_blocks_per_char

def get_stats():
     blocks_array, bitmap_count, name_to_block_grid, max_blocks_per_char = get_12px_blocks()

     for blk in blocks_array:
         for row in blk:
             print ' '.join(map(str, row))
         print "\n============\n"

     blockcount = len(blocks_array)
     uncompressed = bitmap_count*12*12/8
     compressed = blockcount*BLOCK_WIDTH*BLOCK_HEIGHT/8 + bitmap_count*(12/BLOCK_WIDTH)*(12/BLOCK_HEIGHT)
     
     print "There are %i blocks" % blockcount
     print "Maximum blocks per char: %i" % max_blocks_per_char
     print "Size uncompressed %i" % uncompressed
     print "Size compressed %i" % compressed

def output_tables():
    dotc = open("bitmaps.c", "w")
    doth = open("bitmaps.h", "w")
    doth.write("#ifndef BITMAPS_H\n#define BITMAPS_H\n\n")

    c = [0]
    def output_bit(b):
        if c[0] != 0 and c[0] % 8 == 0:
            dotc.write(",0b")
        dotc.write(str(b))
        c[0] += 1
    dotc.write("const uint8_t CHAR_BLOCKS_12PX[] PROGMEM = {\n    ")
    blocks_array, bitmap_count, name_to_block_grid, max_blocks_per_char = get_12px_blocks()
    for b in blocks_array:
        for r in b:
            for bit in r:
                output_bit(bit)
#            dotc.write('    ' + ', '.join(map(str, r)) + ',\n')
        dotc.write("\n    ")
    dotc.write("\n};\n")

    dotc.write('const uint8_T CHAR_12PX_GRIDS[] PROGMEM = {\n')
    i = 0
    for name, grid in name_to_block_grid.iteritems():
        m = re.match(r"^12px_([^.]+)\.png$", name)
        assert m
        cname = 'CHAR_12PX_' + m.group(1).upper()
        doth.write('#define ' + cname + ' (CHAR_12PX_GRIDS + ' + str(i) + ')\n')
        for row in grid:
            dotc.write('    ' + ', '.join(map(str, row)) + ',\n')
        dotc.write('\n')
        i += 16
    dotc.write('};\n')

    doth.write("\n#endif\n")

    dotc.close()
    doth.close()

if __name__ == '__main__':
    assert len(sys.argv) > 1

    if sys.argv[1] == 'stats':
        get_stats()
    elif sys.argv[1] == 'output':
        output_tables()
