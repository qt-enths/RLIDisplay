if ! [ -d 'build' ]; then
	mkdir build
fi
cd build
rm -rf *
#/usr/lib/x86_64-linux-gnu/qt4/bin/qmake ../QtChartGdal.pro -r -spec linux-g++-64 CONFIG+=release
#/opt/qt4/bin/qmake ../RLIDisplay.pro -r -spec linux-g++ CONFIG+=release
/opt/qt4/bin/qmake ../RLIDisplay.pro -r -spec linux-g++ CONFIG+=debug
make
cp -ar ../res res
#cp -ar ../s57attributes.csv s57attributes.csv
#cp -ar ../s57objectclasses.csv s57objectclasses.csv
