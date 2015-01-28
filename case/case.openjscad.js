// title: OpenJSCAD.org Logo
// author: Rene K. Mueller
// license: Creative Commons CC BY
// URL: http://openjscad.org/#examples/logo.jscad
// revision: 0.003
// tags: Logo,Intersection,Sphere,Cube

var THICK_MARGIN = 0.25
var THICK =
    1.75 + // Screen (buttons are slimmer)
    2.75 + // USB connector (highest thing on top layer)
    0.6 +  // Thinnest PCB we can have made cheaply.
    THICK_MARGIN;

var PCB_WIDTH = 26.7;
var PCB_HEIGHT = 33.0708;

var PCB_HORIZ_MARGIN = 0.5;

var WIDTH = PCB_WIDTH + PCB_HORIZ_MARGIN;
var HEIGHT = PCB_HEIGHT + PCB_HORIZ_MARGIN;

var CASE_THICK = 0.5;

var PCB_LEDGE_WIDTH = 2.8;
var PCB_THICK = 0.6;
var SCREEN_THICK = 1.75;
var LEDGE_BOTTOM = PCB_THICK +
                   SCREEN_THICK;

var SENSOR_POSITIONS = [
    [3.81, PCB_HEIGHT-4.4958],
    [6.35, PCB_HEIGHT-4.4958],
    [15.24, PCB_HEIGHT-4.4958],
    [17.78, PCB_HEIGHT-4.4958]
];

var SENSOR_WIDTH = 1.6;
var SENSOR_HEIGHT = SENSOR_WIDTH;

var SCREEN_FROM_TOP = 2;
var SCREEN_WIDTH = 26.7;
var SCREEN_HEIGHT = 19.26;

function make_box(w,h,t) {
    var obox2d = hull(
        circle(t/2),
        circle(t/2).translate([w,0,0])
    );
    var obox = linear_extrude({ height: h }, obox2d);
    return obox.rotateX(90);
}

function main() {
    var obox = make_box(WIDTH, HEIGHT+CASE_THICK, THICK+CASE_THICK*2);
    var ibox = make_box(WIDTH, HEIGHT, THICK).translate([CASE_THICK, CASE_THICK, CASE_THICK]);
    var ledge = cube({
        size: [
            PCB_LEDGE_WIDTH+THICK-0.2,
            HEIGHT,
            1.0
        ]
    }).translate([0.2, -HEIGHT, THICK-LEDGE_BOTTOM]);

    var box = obox.subtract(ibox).union(ledge);
    for (var i = 0; i < SENSOR_POSITIONS.length; ++i) {
        var x = SENSOR_POSITIONS[i][0];
        var y = SENSOR_POSITIONS[i][1];

        box = box.subtract(
            cube({size: [ SENSOR_WIDTH, SENSOR_HEIGHT, THICK ]}).translate([x+THICK, -y, 0])
        );
    }

    box = box.subtract(cube({
        size: [ SCREEN_WIDTH, SCREEN_HEIGHT, CASE_THICK ]
    }).translate([  , -(HEIGHT-SCREEN_FROM_TOP), THICK+CASE_THICK ]));

    return box;
}
