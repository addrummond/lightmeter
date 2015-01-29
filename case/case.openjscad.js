var THICK_MARGIN = 0.25;
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
var SENSOR_HOLE_EXTRA = 2;

var SCREEN_FROM_TOP = 2;
var SCREEN_WIDTH = 26.7;
var SCREEN_HEIGHT = 19.26;

var TOTAL_WIDTH = WIDTH + THICK + (CASE_THICK*2);

var BIG_BUTTON_WIDTH = 13;
var BIG_BUTTON_HEIGHT = 8;
var BIG_BUTTON_THICK = 1;
var BIG_BUTTON_NOB_HEIGHT = 1;
var BIG_BUTTON_NOB_RAD = 1.5;

var BIG_BUTTON_FROM_BOTTOM = 2;

function make_box(w,h,t) {
    var obox2d = hull(
        circle(t/2),
        circle(t/2).translate([w,0,0])
    );
    var obox = linear_extrude({ height: h }, obox2d);
    return obox.rotateX(90);
}

function make_big_button(pad){
    var top = linear_extrude({ height: BIG_BUTTON_THICK }, square([BIG_BUTTON_WIDTH+pad, BIG_BUTTON_HEIGHT+pad]));
    var nob = linear_extrude({ height: BIG_BUTTON_NOB_HEIGHT }, circle(BIG_BUTTON_NOB_RAD))
    .translate([BIG_BUTTON_WIDTH/2-BIG_BUTTON_NOB_RAD, BIG_BUTTON_HEIGHT/2-BIG_BUTTON_NOB_RAD, -BIG_BUTTON_THICK]);
    var but = top.union(nob);
    return but;
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
    var h = [ ];
    for (var i = 0; i < SENSOR_POSITIONS.length; ++i) {
        var x = SENSOR_POSITIONS[i][0];
        var y = SENSOR_POSITIONS[i][1];
        h.push(square([SENSOR_WIDTH+SENSOR_HOLE_EXTRA, SENSOR_HEIGHT+SENSOR_HOLE_EXTRA])
        .translate([x+THICK/2+CASE_THICK,-y]));
    }

    var sensor_hole = linear_extrude({ height: CASE_THICK }, chain_hull(h));
    box = box.subtract(sensor_hole);

    box = box.subtract(cube({
        size: [ SCREEN_WIDTH, SCREEN_HEIGHT, CASE_THICK ]
    }).translate([THICK/2+CASE_THICK, -(HEIGHT-SCREEN_FROM_TOP), THICK+CASE_THICK ]));

    return box.union(make_big_button().translate([30,30      ,0]));
}
