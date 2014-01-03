# only work on MacOSX(tested on 10.9)
WORKDIR=`mktemp -d -t otamainstall`
echo $WORKDIR

cd $WORKDIR
git clone https://github.com/nagadomi/eiio.git
cd eiio
./autogen.sh
./configure --disable-gif && make && make install

cd $WORKDIR
git clone https://github.com/nagadomi/nv.git
cd nv
./autogen.sh
./configure --disable-openmp && make && make install

cd $WORKDIR
git clone https://github.com/nagadomi/otama.git
cd otama
./autogen.sh
./configure --disable-openmp --disable-leveldb --without-ruby
make && make install

rm -rf $WORKDIR
