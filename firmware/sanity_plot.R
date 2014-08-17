require('ggplot2')

num_stages <- 6

plot_sanity_graph <- function () {
    d <- read.csv("sanitygraph.csv")
    num_stages <- (ncol(d) - 1) / 2
    plot = ggplot(data=d)
    for (i in 1:num_stages) {
        plot = plot + geom_line(aes_string(x="v", y=paste("s", as.character(i), sep="")))
    }
    plot
}
