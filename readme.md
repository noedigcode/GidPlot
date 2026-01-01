![GidPlot Logo](images/gidplot_64.png)
Gid~Plot
========

An app to just plot CSV data
----------------------------

2024-2026 Gideon van der Kolf, noedigcode@gmail.com

Just import a CSV file and plot some data. Is that so hard?

And show the CSV data in a table, mark import errors, link different plots,
create sub-ranges to plot, snap windows to screen edges and a bit more.

Plus, plot latitude/longitude data on a map. Yes, an actual map!

- [QCustomPlot](https://www.qcustomplot.com/) is used for plotting.
- [QGeoView](https://github.com/AmonRaNet/QGeoView) is used for map plotting.
- Map tiles are retrieved on the fly from [OpenStreetMap](https://www.openstreetmap.org/).


Requirements:
-------------

- Qt 5
- Windows or Linux (and probably Mac too)
- OpenSSL for retrieving map tiles over https.
  - On Windows, you need the `libcrypto-1_1-x64.dll` and `libssl-1_1-x64.dll` files
    which are not provided by Qt. You need to build these yourself or find them
    bundled with some application.


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

