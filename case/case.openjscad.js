var NFACES = 20;

var THICK_MARGIN = 0.15;
var SCREEN_THICK = 1.75;

var THICK =
SCREEN_THICK + // Screen (buttons are slimmer)
1.76 + // USB connector (highest thing on top layer; mid mount)
0.6 +  // Thinnest PCB we can have made cheaply.
THICK_MARGIN;

var CASE_THICK = 0.71;

var TOTAL_THICKNESS = THICK + CASE_THICK*2;
console.log("Total thickness: " + TOTAL_THICKNESS);

var SCREEN_FROM_TOP = 2;
var SCREEN_WIDTH = 26.7;
var SCREEN_HEIGHT = 19.26;

var PCB_WIDTH = SCREEN_WIDTH + 1;
var PCB_HEIGHT = 38.1508;
var PCB_HORIZ_MARGIN = 0.5;

var BATTERY_HEIGHT = 19.75;
var BATTERY_THICK = 3.8;

var WIDTH = PCB_WIDTH + PCB_HORIZ_MARGIN;
var HEIGHT = PCB_HEIGHT + PCB_HORIZ_MARGIN;

var PCB_LEDGE_WIDTH = 2.8;
var PCB_THICK = 0.6;
var SCREEN_THICK = 1.75;

var SENSOR_POSITIONS = [
[3.81, 4.4958],
[6.35, 4.4958],
[15.24, 4.4958],
[17.78, 4.4958]
];

var SENSOR_WIDTH = 1.6;
var SENSOR_HEIGHT = SENSOR_WIDTH;
var SENSOR_HOLE_EXTRA = 2;

var total_incident_sensor_width = SENSOR_POSITIONS[3][0] - SENSOR_POSITIONS[2][0] + SENSOR_WIDTH;

var SPHERE_HEIGHT = total_incident_sensor_width/1.4;
var SPHERE_RECESS = 1.5;
var SPHERE_THICK = 0.71;

var SPHERE_HOLE_HEIGHT = Math.sqrt(SPHERE_HEIGHT*SPHERE_HEIGHT - SPHERE_RECESS*SPHERE_RECESS);

var TOTAL_WIDTH = WIDTH + THICK + (CASE_THICK*2);

var BIG_BUTTON_WIDTH = 11;
var BIG_BUTTON_HEIGHT = 8;
var BIG_BUTTON_THICK = 1;
var BIG_BUTTON_NOB_HEIGHT = 1;
var BIG_BUTTON_NOB_RAD = 1.5;
var BIG_BUTTON_CENTER_FROM_BOTTOM = 2.667;
var BUTTON_CENTER_DIV = 3;

var BUTTON_HOLE_MARGIN = 0.1;

var USB_GAP_BTM_FROM_BTM = 19.05;
var USB_GAP_TOP_FROM_BTM = 31.115;

function make_hexagon(radius) {
    var sqrt3 = Math.sqrt(3)/2;
    var hex = CSG.Polygon.createFromPoints([
        [radius, 0, 0],
        [radius / 2, radius * sqrt3, 0],
        [-radius / 2, radius * sqrt3, 0],
        [-radius, 0, 0],
        [-radius / 2, -radius * sqrt3, 0],
        [radius / 2, -radius * sqrt3, 0]
        ]);
    }

    function make_sphere(h) {
        return sphere(h);
    }

    function make_hollow_half_sphere() {
        var so = make_sphere(SPHERE_HEIGHT + SPHERE_THICK);
        var si = make_sphere(SPHERE_HEIGHT);
        var s = so.subtract(si);
        var cb = cube({size: [SPHERE_HEIGHT*2.7, SPHERE_HEIGHT*2.7, SPHERE_HEIGHT*2]}).translate([-SPHERE_HEIGHT*1.4,-SPHERE_HEIGHT*1.4,0]);
        return s.subtract(cb);
    }

    function make_box(w,h,t,case_thick) {
        var rect = cube({size: [w,h,t]});
        rect = expand(case_thick, NFACES, rect);
        return rect;
    }

    function make_hollow_box(w, h, t, case_thick) {
        var obox = make_box(w, h, t, case_thick);
        var ibox = cube({size: [w,h+case_thick,t]});

        var box = obox.subtract(ibox);
        var ledge = cube({
            size: [
            PCB_LEDGE_WIDTH,
            HEIGHT,
            THICK-SCREEN_THICK-PCB_THICK
            ]
        }).translate([0.2, 0, 0]);
        box = box.union(ledge);
        return box;
    }

    function make_big_button(pad){
        var top = linear_extrude({ height: BIG_BUTTON_THICK }, expand(1, NFACES, square([BIG_BUTTON_WIDTH+pad*2, BIG_BUTTON_HEIGHT+pad*2])));
        var nob = linear_extrude({ height: BIG_BUTTON_NOB_HEIGHT }, circle(BIG_BUTTON_NOB_RAD))
        .translate([BIG_BUTTON_WIDTH/2-BIG_BUTTON_NOB_RAD, BIG_BUTTON_HEIGHT/2-BIG_BUTTON_NOB_RAD, -BIG_BUTTON_THICK]);
        var but = top.union(nob);
        return but;
    }

    var bigbutfromtop = HEIGHT - BIG_BUTTON_CENTER_FROM_BOTTOM - BIG_BUTTON_HEIGHT/BUTTON_CENTER_DIV;

    function output_big_button() {
        return color("black", make_big_button(0)
        .translate([(WIDTH-BIG_BUTTON_WIDTH)/2, bigbutfromtop, THICK-BIG_BUTTON_THICK+CASE_THICK]));
    }

    function output_case() {
        var box = make_hollow_box(WIDTH, HEIGHT+BATTERY_HEIGHT, THICK, CASE_THICK);
        var h = [ ];
        for (var i = 2; i < 4; ++i) {
            var x = SENSOR_POSITIONS[i][0];
            var y = SENSOR_POSITIONS[i][1];
            h.push(square([SENSOR_WIDTH+SENSOR_HOLE_EXTRA, SENSOR_HEIGHT+SENSOR_HOLE_EXTRA])
            .translate([(WIDTH-x)+CASE_THICK-(SENSOR_WIDTH+SENSOR_HOLE_EXTRA),y+CASE_THICK-((SENSOR_HEIGHT+SENSOR_HOLE_EXTRA)/2)]));
        }

        var sensor_hole = linear_extrude({ height: CASE_THICK }, chain_hull(h));
        sensor_hole = sensor_hole.translate([0,0,-CASE_THICK]);
        box = box.subtract(sensor_hole);

        var ipos = [((WIDTH-SENSOR_POSITIONS[0][0]) + (WIDTH-SENSOR_POSITIONS[1][0]))/2.0 + CASE_THICK, SENSOR_POSITIONS[0][1]+CASE_THICK, 0];
        var incident_hole = make_sphere(SPHERE_HOLE_HEIGHT-0.1).translate(ipos);
        box = box.subtract(incident_hole);
        var hs = make_hollow_half_sphere().translate(ipos).translate([0,0,SPHERE_RECESS]);
        hs = hs.subtract(cube({size: [SPHERE_HEIGHT*2.6,SPHERE_HEIGHT*2.6,SPHERE_HEIGHT*2.6]}).translate(ipos).translate([-SPHERE_HEIGHT*1.3, -SPHERE_HEIGHT*1.3,0.01]));
        box = box.union(hs);

        box = box.subtract(cube({
            size: [ SCREEN_WIDTH, SCREEN_HEIGHT, CASE_THICK ]
        }).translate([(WIDTH-SCREEN_WIDTH)/2, SCREEN_FROM_TOP, THICK ]));

        var bigbuthole = make_big_button(BUTTON_HOLE_MARGIN)
        .translate([(WIDTH-BIG_BUTTON_WIDTH-BUTTON_HOLE_MARGIN*2)/2, bigbutfromtop-BUTTON_HOLE_MARGIN, THICK-BIG_BUTTON_THICK+CASE_THICK]);
        box = box.subtract(bigbuthole);

        return color("red", box);
    }

    function output_height_reference() {
        return cube({size: [10, 10, TOTAL_THICKNESS] })
        .translate([-11,-11,-CASE_THICK]);
    }

    function main() {
        return output_case()
        .union(output_big_button())
        .union(output_height_reference());
    }
