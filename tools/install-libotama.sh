# only work on Ubuntu
WORKDIR=`mktemp -d`
echo $WORKDIR

sudo apt-get install gcc g++ make autoconf libtool libpng-dev libjpeg-dev libgif-dev \
                     libssl-dev libyaml-dev libsqlite3-dev libpq-dev libmysqlclient-dev

cd $WORKDIR
git clone https://github.com/nagadomi/eiio.git
cd eiio
./autogen.sh
./configure && make && sudo make install && sudo ldconfig

cd $WORKDIR
git clone https://github.com/nagadomi/nv.git
cd nv
./autogen.sh
./configure && make && sudo make install && sudo ldconfig

cd $WORKDIR
git clone https://github.com/nagadomi/otama.git
cd otama
./configure --enable-pgsql --enable-mysql
make && make check && sudo make install && sudo ldconfig

rm -rf $WORKDIR
