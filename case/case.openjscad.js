//
// TODO:
//
// * Make body thinner around incident dome.
// * Sensor holes are positioned a bit too low down for some reason.
//

var separated = true;

var NFACES = 20;

var THICK_MARGIN = 0.15;
var SCREEN_THICK = 1.75;
var BUTTON_THICK = 0.5; // 0.35 +/- 0.15

var THICK =
SCREEN_THICK + // Screen (buttons are slimmer)
1.76 + // USB connector (highest thing on top layer; mid mount)
0.6 +  // Thinnest PCB we can have made cheaply.
THICK_MARGIN;

var CASE_THICK = 0.71;

var TOTAL_THICKNESS = THICK + CASE_THICK*2;
console.log("Total thickness: " + TOTAL_THICKNESS);
console.log("Inner thickness: " + THICK);

var SCREEN_FROM_TOP = 3.80917;
var SCREEN_WIDTH = 23.744;
var SCREEN_HEIGHT = 12.864;
var SCREEN_HIDDEN_BORDER = 0.5;
SCREEN_WIDTH -= SCREEN_HIDDEN_BORDER*2;
SCREEN_HEIGHT -= SCREEN_HIDDEN_BORDER*2;

var PCB_WIDTH = SCREEN_WIDTH + 1;
var PCB_HEIGHT = 38.608;
var PCB_HORIZ_MARGIN = 0.5;

var BATTERY_HEIGHT = 19.75;
var BATTERY_THICK = 3.8;

var USB_PORT_WIDTH = 7.9;
var USB_PORT_MARGIN = 1;
var LIP = 0.24; // Estimate from hopefully to scale drawing (!)
var USB_PORT_THICK_ABOVE_PCB_CENTER = 1.18 + 0.6/2 + LIP;
var USB_PORT_THICK_BELOW_PCB_CENTER = 1.52 - 0.6/2;
var USB_PORT_THICK = 2.94;
if (Math.abs((USB_PORT_THICK_ABOVE_PCB_CENTER + USB_PORT_THICK_BELOW_PCB_CENTER) - USB_PORT_THICK > 0.0001))
    throw new Error("Bad value!");

//var USB_CENTER_FROM_TOP = 28.2702; // Value for original prototype board.
var USB_CENTER_FROM_TOP = 25.5524;

var WIDTH = PCB_WIDTH + PCB_HORIZ_MARGIN;
var HEIGHT = PCB_HEIGHT + PCB_HORIZ_MARGIN;

var TOP_OF_BOTTOM_SCREW_FROM_BOTTOM = 1.77 + 1.0/*error margin*/;

var PCB_THICK = 0.6;
var SCREEN_THICK = 1.75;
var PCB_LEDGE_WIDTH = 2.7;
var PCB_LEDGE_THICK = THICK-SCREEN_THICK-PCB_THICK;
console.log("Ledge thickness: " + PCB_LEDGE_THICK);

var SENSOR_POSITIONS = [
[SCREEN_WIDTH-3.81 + (PCB_WIDTH-SCREEN_WIDTH)/2, 3.175],
[SCREEN_WIDTH-6.35 + (PCB_WIDTH-SCREEN_WIDTH)/2, 3.175],
[SCREEN_WIDTH-15.24 + (PCB_WIDTH-SCREEN_WIDTH)/2, 3.175],
[SCREEN_WIDTH-17.78 + (PCB_WIDTH-SCREEN_WIDTH)/2, 3.175]
];

var SENSOR_WIDTH = 1.6;
var SENSOR_HEIGHT = SENSOR_WIDTH;
var SENSOR_HOLE_EXTRA = 2;

var total_incident_sensor_width = SENSOR_POSITIONS[3][0] - SENSOR_POSITIONS[2][0] + SENSOR_WIDTH;

var SPHERE_HEIGHT = total_incident_sensor_width/1.4;
var SPHERE_RECESS = 1.5;
var SPHERE_THICK = 0.71;
var SPHERE_BAND_THICK = 0.5;
var SPHERE_BAND_DIAM = 1;
var SPHERE_HOLE_HEIGHT = Math.sqrt(SPHERE_HEIGHT*SPHERE_HEIGHT - SPHERE_RECESS*SPHERE_RECESS);

var TOTAL_WIDTH = WIDTH + THICK + (CASE_THICK*2);

var BUTTON_PROTRUSION_THICK = 0.2;
var BIG_BUTTON_WIDTH = 11;
var BUTTON_HEIGHT = 8;
var BUTTON_THICK = CASE_THICK + BUTTON_PROTRUSION_THICK;
var BUTTON_NOB_HEIGHT = SCREEN_THICK - BUTTON_THICK;
var BUTTON_NOB_RAD = 1.5;
var BUTTON_CENTER_FROM_BOTTOM = 2.0574000000000003396;
var BUTTON_CENTER_DIV = 3;
var SMALL_BUTTON_WIDTH = 3;
var BUTTON_HOLE_MARGIN = 0.1;

// Mouser part no. 749-901-050
var SPACER_OUTER_DIAM = 1;
var SPACER_MARGIN = 0.2;
var SPACER_HOLE_FROM_TOP = 7.4168;
var SPACER_DEPTH = 3;

function make_hexagon(radius) {
    var sqrt3 = Math.sqrt(3)/2;
    return polygon([
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

function make_spacer_hole() {
    var circ = circle({center: true, r: (SPACER_OUTER_DIAM + SPACER_MARGIN)/2 });
    var hole = linear_extrude({ height: SPACER_DEPTH }, circ);
    //var leftedge = cube({ size: [0.5, SPACER_OUTER_DIAM/2, PCB_LEDGE_THICK+0.1], center: true })
    //               .translate([PCB_LEDGE_WIDTH/2.0, 0, (PCB_LEDGE_THICK+0.1)/2.0]);
    //hole = hole.union(leftedge);
    hole = hole.rotateX(90);
    return color("pink", hole);
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
            HEIGHT - TOP_OF_BOTTOM_SCREW_FROM_BOTTOM,
            PCB_LEDGE_THICK
        ]
    }).translate([0, 0, 0]);
    box = box.union(ledge);
    return box;
}

function make_button(width, pad){
    var top = linear_extrude({ height: BUTTON_THICK }, expand(0.5, NFACES, square([width+pad*2, BUTTON_HEIGHT+pad*2])));
    var nob = linear_extrude({ height: BUTTON_THICK+BUTTON_NOB_HEIGHT }, circle(BUTTON_NOB_RAD))
    .translate([width/2-BUTTON_NOB_RAD, BUTTON_HEIGHT/2 - BUTTON_NOB_RAD - BUTTON_HEIGHT/BUTTON_CENTER_DIV/2, -BUTTON_NOB_HEIGHT]);
    var but = top.union(nob);
    return but;
}

function make_big_button(pad) {
    return make_button(BIG_BUTTON_WIDTH, pad)
}

function make_small_button(pad) {
    return make_button(SMALL_BUTTON_WIDTH, pad);
}

var butfromtop = HEIGHT - BUTTON_CENTER_FROM_BOTTOM - BUTTON_HEIGHT/BUTTON_CENTER_DIV;
var buttonZ = separated ? 10 : 0;

function output_big_button() {
    var big = make_big_button(0)
              .translate([(WIDTH-BIG_BUTTON_WIDTH)/2, butfromtop, THICK-BUTTON_THICK+CASE_THICK + buttonZ]);
    return color("white", big);
}

function output_small_button() {
    var smallleft = make_small_button(0)
                    .translate([WIDTH-CASE_THICK*1.5-SMALL_BUTTON_WIDTH,
                                butfromtop,
                                THICK-BUTTON_THICK+CASE_THICK + buttonZ]);
    return color("white", smallleft);
}

var ipos = [((WIDTH-SENSOR_POSITIONS[0][0]) + (WIDTH-SENSOR_POSITIONS[1][0]))/2.0 + CASE_THICK, SENSOR_POSITIONS[0][1]+CASE_THICK, 0];

function output_incident_dome()
{
    var hs = make_hollow_half_sphere();
    hs = hs.union(linear_extrude({ height: SPHERE_BAND_THICK }, circle({ center:true, r: SPHERE_HOLE_HEIGHT+SPHERE_BAND_DIAM })
                                                                .subtract(circle({ center: true, r: SPHERE_HOLE_HEIGHT })))
                  .translate([0, 0, -SPHERE_RECESS+0.01]));
    hs = hs.subtract(cube({ center: true, size: [SPHERE_HEIGHT*4, SPHERE_HEIGHT*4, SPHERE_RECESS-SPHERE_BAND_THICK ]}).translate([0, 0, -(SPHERE_RECESS-SPHERE_BAND_THICK)/2]));
    hs = hs.translate(ipos).translate([0,0,separated ? -10 : SPHERE_RECESS]);

    return color("white", hs);
}

function output_case() {
    var box = color("red", make_hollow_box(WIDTH, HEIGHT+BATTERY_HEIGHT, THICK, CASE_THICK));
    var h = [ ];
    for (var i = 2; i < 4; ++i) {
        var x = SENSOR_POSITIONS[i][0];
        var y = SENSOR_POSITIONS[i][1];
        h.push(square([SENSOR_WIDTH+SENSOR_HOLE_EXTRA, SENSOR_HEIGHT+SENSOR_HOLE_EXTRA])
        .translate([(WIDTH-x)+CASE_THICK-(SENSOR_WIDTH+SENSOR_HOLE_EXTRA),y+CASE_THICK-((SENSOR_HEIGHT+SENSOR_HOLE_EXTRA)/2)]));
    }

    // Hole for reflective sensors.
    var sensor_hole = linear_extrude({ height: CASE_THICK }, chain_hull(h));
    sensor_hole = sensor_hole.translate([0,0,-CASE_THICK]);
    box = box.subtract(sensor_hole);

    // Hole for incident sensors.
    var incident_hole = make_sphere(SPHERE_HOLE_HEIGHT + SPHERE_THICK).translate(ipos);
    box = box.subtract(incident_hole);

    // Hole for screen.
    box = box.subtract(cube({
        size: [ SCREEN_WIDTH, SCREEN_HEIGHT, CASE_THICK ]
    }).translate([(WIDTH-SCREEN_WIDTH)/2, SCREEN_FROM_TOP, THICK ]));

    // Hole for large center button.
    var bigbuthole = make_big_button(BUTTON_HOLE_MARGIN)
                     .translate([(WIDTH-BIG_BUTTON_WIDTH-BUTTON_HOLE_MARGIN*2)/2,
                                 butfromtop-BUTTON_HOLE_MARGIN,
                                 THICK-BUTTON_THICK+CASE_THICK]);
    box = box.subtract(bigbuthole);

    // Hole for small left/right buttons.
    var smallbutholeright = make_small_button(BUTTON_HOLE_MARGIN)
                           .translate([CASE_THICK*1.5,
                                       butfromtop-BUTTON_HOLE_MARGIN,
                                       THICK-BUTTON_THICK+CASE_THICK]);
    var smallbutholeleft = make_small_button(BUTTON_HOLE_MARGIN)
                           .translate([WIDTH-CASE_THICK*1.5-SMALL_BUTTON_WIDTH-BUTTON_HOLE_MARGIN,
                                       butfromtop-BUTTON_HOLE_MARGIN,
                                       THICK-BUTTON_THICK+CASE_THICK]);
    box = box.subtract(smallbutholeright).subtract(smallbutholeleft);

    // Hole for USB port.
    var port = cube({size: [20, USB_PORT_WIDTH+USB_PORT_MARGIN, USB_PORT_THICK ], center: true })
               .translate([TOTAL_WIDTH,
                           USB_CENTER_FROM_TOP,
                           THICK-SCREEN_THICK-PCB_THICK/2-USB_PORT_THICK_BELOW_PCB_CENTER+CASE_THICK]);
    box = box.subtract(port);

    box = box.subtract(make_spacer_hole().translate([PCB_LEDGE_WIDTH/2,HEIGHT+0.001-TOP_OF_BOTTOM_SCREW_FROM_BOTTOM,PCB_LEDGE_THICK/2]));

    return box;
}

function output_height_reference() {
    return cube({size: [10, 10, TOTAL_THICKNESS] })
    .translate([-11,-11,-CASE_THICK]);
}

function main() {
    return union(
        output_case(),
        output_big_button(),
        output_small_button(),
        output_incident_dome()
        //output_height_reference()
    );
}
