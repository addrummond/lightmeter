require('ggplot2')

plotgraph <- function () {
    d = read.csv("sanitygraph.csv")
    plot = ggplot(data=d)
    for (i in 1:3) {
        plot = plot + geom_line(aes_string(x="v", y=paste("s", as.character(i), sep="")))
    }
    plot
}