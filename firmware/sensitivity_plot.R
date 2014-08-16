require("ggplot2")

plot_graph <- function () {
    d <- read.csv("sensitivity_curves.csv");

    # Normalize for relative spectral sensitivity of 1.0 at wavelength=550
    diodenorm <- d[d$wavelength==555,"diode.relative.spectral.sensitivity"];
    qb12norm <- d[d$wavelength==555,"qb12.relative.spectral.sensitivity"];
    qb11norm <- d[d$wavelength==555,"qb11.relative.spectral.sensitivity"];
    d["qb12normed"] <- d$qb12.relative.spectral.sensitivity / qb12norm;
    d["qb11normed"] <- d$qb11.relative.spectral.sensitivity / qb11norm;
    d["diodenormed"] <- d$diode.relative.spectral.sensitivity / diodenorm;

    p <- ggplot(d, aes(x=wavelength)) +
         geom_line(aes(y=luminosity.func,color="Luminosity function")) +
         geom_line(aes(y=diodenormed,color="Diode without filters")) +
         geom_line(aes(y=qb12normed,color="Diode with QB12 filter")) +
         geom_line(aes(y=qb11normed,color="Diode with QB11 filter")) +
         labs(x="Wavelength (nm)", y="Normalized relative spectral sensitivity (1.0 at 555nm)");

    p;
}
