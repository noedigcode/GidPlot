![GidPlot Logo](images/gidplot_64.png)
GidPlot
========

An app to just plot CSV data
----------------------------

2024 Gideon van der Kolf, noedigcode@gmail.com

Just import a CSV file and plot some data. Is that so hard?

And show the CSV data in a table, mark import errors, link different plots,
create sub-ranges to plot, snap windows to screen edges and a bit more.

[QCustomPlot](https://www.qcustomplot.com/) is used for plotting.

Requirements:
-------------

* Qt 5
* Windows or Linux (and probably Mac too)

Building:
---------

Open the gidplot.pro file with QtCreator and build

Or run the following from the command line:
```
mkdir build
cd build
qmake ../gidplot.pro
make
```

