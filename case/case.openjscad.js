var THICK_MARGIN = 0.25;
var THICK =
1.75 + // Screen (buttons are slimmer)
2.75 + // USB connector (highest thing on top layer)
0.6 +  // Thinnest PCB we can have made cheaply.
THICK_MARGIN;

var PCB_WIDTH = 26.7;
var PCB_HEIGHT = 38.1508;
var PCB_HORIZ_MARGIN = 0.5;

var BATTERY_HEIGHT = 19.75;
var BATTERY_THICK = 3.8;

var WIDTH = PCB_WIDTH + PCB_HORIZ_MARGIN;
var HEIGHT = PCB_HEIGHT + PCB_HORIZ_MARGIN;

var CASE_THICK = 0.5;

var PCB_LEDGE_WIDTH = 2.8;
var PCB_THICK = 0.6;
var SCREEN_THICK = 1.75;
var LEDGE_BOTTOM = PCB_THICK + SCREEN_THICK;

var SENSOR_POSITIONS = [
[3.81, 4.4958],
[6.35, 4.4958],
[15.24, 4.4958],
[17.78, 4.4958]
];

var SENSOR_WIDTH = 1.6;
var SENSOR_HEIGHT = SENSOR_WIDTH;
var SENSOR_HOLE_EXTRA = 2;

var SCREEN_FROM_TOP = 2;
var SCREEN_WIDTH = 26.7;
var SCREEN_HEIGHT = 19.26;

var TOTAL_WIDTH = WIDTH + THICK + (CASE_THICK*2);

var BIG_BUTTON_WIDTH = 11;
var BIG_BUTTON_HEIGHT = 8;
var BIG_BUTTON_THICK = 1;
var BIG_BUTTON_NOB_HEIGHT = 1;
var BIG_BUTTON_NOB_RAD = 1.5;
var BIG_BUTTON_CENTER_FROM_BOTTOM = 2.667;
var BUTTON_CENTER_DIV = 3;

var BUTTON_HOLE_MARGIN = 0.1;

function make_box(w,h,t,case_thick) {
    var rect = cube({size: [w,h,t]});
    rect = expand(case_thick, 12, rect);
    return rect;
}

function make_hollow_box(w, h, t, case_thick, ledge_height) {
    var obox = make_box(w, h, t, case_thick);
    var ibox = cube({size: [w,h+case_thick,t]});

    var box = obox.subtract(ibox);
    if (ledge_height) {
        var ledge = cube({
            size: [
            PCB_LEDGE_WIDTH,
            ledge_height,
            1.0
            ]
        }).translate([0.2, 0, t-LEDGE_BOTTOM]);
        box = box.union(ledge);
    }
    return box;
}

function make_big_button(pad){
    var top = linear_extrude({ height: BIG_BUTTON_THICK }, square([BIG_BUTTON_WIDTH+pad, BIG_BUTTON_HEIGHT+pad]));
    var nob = linear_extrude({ height: BIG_BUTTON_NOB_HEIGHT }, circle(BIG_BUTTON_NOB_RAD))
    .translate([BIG_BUTTON_WIDTH/2-BIG_BUTTON_NOB_RAD, BIG_BUTTON_HEIGHT/2-BIG_BUTTON_NOB_RAD, -BIG_BUTTON_THICK]);
    var but = top.union(nob);
    return but;
}

function main() {
    var box = make_hollow_box(WIDTH, HEIGHT+BATTERY_HEIGHT, THICK, CASE_THICK, HEIGHT);
    var h = [ ];
    for (var i = 0; i < SENSOR_POSITIONS.length; ++i) {
        var x = SENSOR_POSITIONS[i][0];
        var y = SENSOR_POSITIONS[i][1];
        h.push(square([SENSOR_WIDTH+SENSOR_HOLE_EXTRA, SENSOR_HEIGHT+SENSOR_HOLE_EXTRA])
        .translate([x+THICK/2+CASE_THICK,y]));
    }

    var sensor_hole = linear_extrude({ height: CASE_THICK }, chain_hull(h));
    sensor_hole = sensor_hole.translate(-CASE_THICK);
    box = box.subtract(sensor_hole);

    box = box.subtract(cube({
        size: [ SCREEN_WIDTH, SCREEN_HEIGHT, CASE_THICK ]
    }).translate([0, SCREEN_FROM_TOP, THICK ]));

    var bigbutfromtop = HEIGHT - BIG_BUTTON_CENTER_FROM_BOTTOM - BIG_BUTTON_HEIGHT/BUTTON_CENTER_DIV;

    var bigbuthole = make_big_button(BUTTON_HOLE_MARGIN)
    .translate([(WIDTH-BIG_BUTTON_WIDTH-BUTTON_HOLE_MARGIN*2)/2, bigbutfromtop-BUTTON_HOLE_MARGIN, THICK-BIG_BUTTON_THICK+CASE_THICK]);
    box = box.subtract(bigbuthole);

    var cub = cube({size: [20,20,20]});
    var bigbut = make_big_button(0)
    .translate([(WIDTH-BIG_BUTTON_WIDTH)/2, bigbutfromtop, THICK-BIG_BUTTON_THICK+CASE_THICK]);

    return box.union(bigbut);
}
