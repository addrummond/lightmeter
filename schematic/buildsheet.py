from lxml import etree as ET
from reportlab.pdfgen import canvas
from reportlab.lib import colors

class BoardInfo():
    def __init__(self):
        self.value_to_components = { }
        self.name_to_component = { }
        self.layer_value_to_components = { }
        self.components = [ ]

class Component():
    def __init__(self, name, value, pads):
        self.name = name
        self.value = value
        self.pads = pads

class Pad():
    def __init__(self, x, y, width, height):
        self.x = x
        self.y = y
        self.width = width
        self.height = height

def getfloat(elem, name):
    f = elem.get(name, None)
    if f is None:
        raise Exception("Expecting attribute '%s'" % name)
    v = None
    try:
        v = float(f)
    except ValueError:
        raise Exception("Non-float value for attribute '%s'" % name)
    return v


def getBoardInfo(filename):
    bi = BoardInfo()

    tree = ET.parse(filename)
    root = tree.getroot()

    # Get the number of the dimension layer.
    dl = root.xpath("//layer[@name='Dimension']")
    if len(dl) == 0:
        raise Exception("Could not find Dimension layer def")
    bi.dimension_layer_number = dl[0].get('number', 20)

    # Get the numbers of the top and bottom layers.
    tl = root.xpath("//layer[@name='Top']")
    if (len(tl) == 0):
        raise Exception("Could not find Top layer def")
    bi.top_layer_number = dl[0].get('number', 1)
    bl = root.xpath("//layer[@name='Bottom']")
    if (len(bl) == 0):
        raise Exception("Could not find Bottom layer def")
    bi.bottom_layer_number = bl[0].get('number', 16)

    # Get the bounding rectangle from the dimension layer wires.
    ws = root.xpath("//wire[@layer='20']")
    if len(ws) < 2:
        raise Exception("Error processing dimension layer wires")
    xmin, ymin = float('+inf'), float('+inf')
    xmax, ymax = float('-inf'), float('-inf')
    for w in ws:
        xmin = min(xmin, getfloat(w, "x1"), getfloat(w, "x2"))
        ymin = min(ymin, getfloat(w, "y1"), getfloat(w, "y2"))
        xmax = max(xmax, getfloat(w, "x1"), getfloat(w, "x2"))
        ymax = max(ymax, getfloat(w, "y1"), getfloat(w, "y2"))
    bi.xmin = xmin
    bi.xmax = xmax
    bi.ymin = ymin
    bi.ymax = ymax
    bi.width = xmax - xmin
    bi.height = ymax - ymin

    # Get components.
    comps = root.xpath("//element")
    ourcomps = [ ]
    for c in comps:
        compname = c.get("name", None)
        if compname is None:
            raise Exception("Component without name")
        compvalue = c.get("value", None)
        if compvalue is None:
            raise Exception("Component without value")

        package_name = c.get("package", None)
        if package_name is None:
            raise Exception("Component without package")
        # Find the package.
        package = root.xpath("//package[@name='%s']" % package_name)
        if len(package) == 0:
            raise Exception("Could not find package")
        ppads = package[0].xpath("smd")
        if len(ppads) == 0:
            print("Skipping package '%s' (component '%s') since it has no pads" % (package_name, compname))
            continue

        compx = getfloat(c, "x")
        compy = getfloat(c, "y")

        ourpads = [ ]
        complayer = None # Assume that all components pads will be on same layer.
        for p in ppads:
            if p.get("layer", None) is None:
                raise Exception("Could not get pad layer")
            complayer = p.get("layer")
            op = Pad(x = getfloat(p, "x") + compx,
                     y = getfloat(p, "y") + compy,
                     width = getfloat(p, "dx"),
                     height = getfloat(p, "dy"))
            ourpads.append(op)

        com = Component(
            name=compname,
            value=compvalue,
            pads=ourpads
        )
        ourcomps.append(com)

        bi.name_to_component[compname] = com
        if compvalue in bi.value_to_components:
            bi.value_to_components[compvalue].append(com)
        else:
            bi.value_to_components[compvalue] = [com]
        if (complayer, compvalue) in bi.layer_value_to_components:
            bi.layer_value_to_components[(complayer, compvalue)].append(com)
        else:
            bi.layer_value_to_components[(complayer, compvalue)] = [com]

    bi.components = ourcomps
    return bi

def render_components(cv, bi, on_layer, with_value):
    for c in bi.components:
        if c in bi.layer_value_to_components[(on_layer, with_value)]:
            continue
        for p in c.pads:
            cv.setFillColor(colors.black)
            cv.rect(p.x - (p.width/2), p.y - (p.height/2),
                    p.width, p.height,
                    stroke=0, fill=1)

    for c in bi.layer_value_to_components[(on_layer, with_value)]:
        for p in c.pads:
            cv.setFillColor(colors.red)
            cv.rect(p.x - (p.width/2), p.y - (p.height/2),
                    p.width, p.height,
                    stroke=0, fill=1)

i = getBoardInfo("lightmeter.brd")
print("Width %f, height %f" % (i.width, i.height))
cv = canvas.Canvas("test.pdf",pagesize=(i.width, i.height))
render_components(cv, i, "1", "10K")
cv.showPage()
cv.save()
