import png
import os

#
# We compress 12x12 digits by splitting them into 3x3 blocks and
# hoping that the number of unique 3x3 patterns will be small. (The
# blocky design of the digits helps with this.)  An x/y offset is
# specified for each character, and if the offset is non-zero the last
# blocks of the grid wrap.
#
# Following pypng, we treat images as lists of lists (lists of rows).
#

def get_unique_3x3_blocks(image, offset, width=12, height=12):
    assert offset <= 2

    blocks = { }
    coords = [ ]
    for y in xrange(0, height, 3):
        for x in xrange(0, width, 3):
            block = [ [0, 0, 0], [0, 0, 0], [0, 0, 0] ]
            by = 0
            for yy in xrange(y, y+3):
                if yy >= height:
                    yy -= height
                bx = 0
                for xx in xrange(x, x+3):
                    if xx >= width:
                        xx -= width
                    block[by][bx] = image[yy][xx]
                    bx += 1
                by += 1
            blocks[str(block)] = block
            coords.append([y, x, str(block)])

    bkeys = blocks.keys()
    bkeys.sort()

    return [blocks[k] for k in bkeys]

bs = [ ]
for name in os.listdir("./"):
    if name.startswith("12px_"):
        r = png.Reader(file=open(name))
        img = r.read()
        width = img[0]
        height = img[1]
        pixels = map(lambda y: map(lambda x: x and 1 or 0, y), img[2])
        bs.append(get_unique_3x3_blocks(pixels, 0, width, height))

for b in bs:
    for blk in b:
        for row in blk:
            print "%s %s %s" % (row[0], row[1], row[2])
        print "\n============\n"

