#!/usr/bin/env bash

# Reset in case getopts has been used previously in the shell.
OPTIND=1

# version without prefix 'v'
ver=

# mozart root dir.
mozart_dir=

# product name
product=

# storage type
storage=

# update method: update_once or update_block
method=

# gen filelist for ota
# args:
#   $1: file: add file to filelist
#   $2: path: where file locate
#   $3: target: /dev/mtdblockX or a abspath.
gen_ota_filelist()
{
	if [ -f $2/$1 ]; then
		# md5
		$mozart_dir/output/host/usr/bin/inirw -f $2/filelist.ini -w -s $1 -k md5 -v `md5sum $2/$1 | tr -s ' ' | cut -d' ' -f1`
		# size
		$mozart_dir/output/host/usr/bin/inirw -f $2/filelist.ini -w -s $1 -k size -v `ls -l $2/$1 | tr -s ' ' | cut -d' ' -f5`
		# target
		$mozart_dir/output/host/usr/bin/inirw -f $2/filelist.ini -w -s $1 -k target -v $3
	fi
}

gen_spinor_ota_filelist()
{
	# 1. add updater to filelist.ini
	gen_ota_filelist updater $target_dir /dev/mtdblock4
	# 2. add uboot to filelist.ini
	gen_ota_filelist uboot $target_dir /dev/mtdblock0
	# 3. add usrdata to filelist.ini
	gen_ota_filelist usrdata $target_dir /dev/mtdblock2
	# 4. add kernel to filelist.ini
	gen_ota_filelist kernel $target_dir /dev/mtdblock3
	# 5. add app to filelist.ini
	gen_ota_filelist appfs $target_dir /dev/mtdblock5
}

gen_spinand_ota_filelist()
{
	# 1. add updater to filelist.ini
	gen_ota_filelist updater $target_dir /dev/mtdblock4
	# 2. add uboot to filelist.ini
	gen_ota_filelist uboot $target_dir /dev/mtdblock0
	# 4. add kernel to filelist.ini
	gen_ota_filelist kernel $target_dir /dev/mtdblock3
	# 5. add app to filelist.ini
	gen_ota_filelist appfs $target_dir /dev/mtdblock5
}

gen_emmc_ota_filelist()
{
	# 1. add updater to filelist.ini
	gen_ota_filelist updater $target_dir /dev/mmcblk0p5
	# 2. add uboot to filelist.ini
	gen_ota_filelist uboot $target_dir /dev/mmcblk0p1
	# 4. add kernel to filelist.ini
	gen_ota_filelist kernel $target_dir /dev/mmcblk0p4
	# 5. add app to filelist.ini
	gen_ota_filelist appfs $target_dir /dev/mmcblk0p6
}

gen_usage()
{
	echo "`basename $0` - a tool to gen ota package"
	echo
	echo "Usage:"
	echo "   `basename $0` <-v version> <-m mozart_dir> <-o output_dir> <-s storage> <-p product>"
	echo
	echo "Example:"
	echo "   `basename $0` -v v1.00001 -m /develop/mozart -o output/updatepkg -s emmc -p canna_v1.0"
	echo
	echo "Option:"
	echo "   v: version, decimal point followed by 5 digits."
	echo "   m: mozart rot dir."
	echo "   o: ota package output dir, target dir will be output_dir/version/."
	echo "   s: storage type"
	echo "   p: product name"
}


# main
while getopts "hv:m:o:s:p:" opt; do
	case "$opt" in
		h)
			gen_usage $0
			exit 0
			;;
		v)
			ver=$OPTARG
			;;
		m)
			mozart_dir=$OPTARG
			;;
		o)
			output_dir=$OPTARG
			;;
		s)
			storage=$OPTARG
			;;
		p)
			product=$OPTARG
			;;
	esac
done


# check version
if [[ $ver =~ ^[0-9]+\.[0-9]{1,5}+\.[0-9]{1,5}$ ]]; then
	version="$ver"
else
	echo
	echo "ERROR: Bad version format"
	gen_usage $0
	exit 1
fi


# check mozart dir
if [ ! -d $mozart_dir ]; then
	echo "ERROR: Invalid mozart dir"
	gen_usage $0
	exit 1
fi

# check product name
if [ ! -n $product ]; then
	echo "ERROR: empty product"
	gen_usage $0
	exit 1
fi

# check storage type
if [ ! -n $storage ]; then
	echo "ERROR: empty storage"
	gen_usage $0
	exit 1
fi

# check target dir
target_dir=$output_dir/$product/v$version
if [ -d $target_dir ]; then
	rm -rf $target_dir
fi
mkdir -p $target_dir

# cp everything to target dir
if [ ! -f $mozart_dir/output/updatepkg/temp/updater.* ]; then
	echo "`basename $0` Error: updater NOT Found."
	exit 1
else
	cp -a $mozart_dir/output/updatepkg/temp/updater.* $target_dir/updater
fi
if [ ! -f $mozart_dir/output/updatepkg/temp/appfs.* ]; then
	echo "`basename $0` Error: appfs NOT Found."
	exit 1
else
	cp -a $mozart_dir/output/updatepkg/temp/appfs.* $target_dir/appfs
fi
if [ ! -f $mozart_dir/tools/host-tools/update_pack/images/*Image ]; then
	echo "`basename $0` Error: kernel NOT Found."
	exit 1
else
	cp -a $mozart_dir/tools/host-tools/update_pack/images/*Image $target_dir/kernel
fi

if [ -f $mozart_dir/output/updatepkg/temp/usrdata.* ]; then
	cp -a $mozart_dir/output/updatepkg/temp/usrdata.* $target_dir/usrdata
fi
if [ -f $mozart_dir/tools/host-tools/update_pack/images/u-boot* ]; then
	cp -af $mozart_dir/tools/host-tools/update_pack/images/u-boot* $target_dir/uboot
fi

# gen filelist.ini
echo
echo "gen otafiles for $product - $storage - v$version"
if [ "$storage" == "spinor" ]; then
	gen_spinor_ota_filelist
elif [ "$storage" == "spinand" ]; then
	gen_spinand_ota_filelist
elif [ "$storage" == "emmc" ]; then
	gen_emmc_ota_filelist
else
	echo -e "storage $storage not support.\n"
	exit -1
fi

# gen info.ini for product
$mozart_dir/output/host/usr/bin/inirw -f $output_dir/$product/info.ini -w -s update -k version -v $version
when=`date +"%a, %d %b %Y %T %Z"`
$mozart_dir/output/host/usr/bin/inirw -f $output_dir/$product/info.ini -w -s update -k time -v "$when"


