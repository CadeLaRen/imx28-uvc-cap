#! /bin/sh

#yll>>
#一般U盘插入之后会在/dev下产生一个sda和一个sda1节点，
#但是只有sda1是可以挂载的。sda是设备，sda1是分区
#有的U盘只会产生一个/dev/sda节点，此时就应该挂载sda。
#这个脚本不区分设备节点和分区节点，一律先挂载，
#但是挂载失败之后会立即清除建立的挂载点
#-------------------------------------------------#
#磁盘种类		设备节点			设备分区节点
#U盘			sd[a-z]				sd[a-z][0-9]
#SD卡			mmcblk[0-9]			mmcblk[0-9]p[0-9]
#CF卡			hd[a-z]				hd[a-z][0-9]
#-------------------------------------------------#
#说明：在这个脚本中的可以把udev脚本的ACTION作为环境变量来用，
#但是无法使用KERNEL
#-------------------------------------------------#
if [ $# -ne 2 ] ; then	#输入参数必须是两个
	exit 1
fi

MNT_PATH=/media/		#挂载移动设备的目录
DEV_PRE=$1				#参数1：挂载点的前缀
DEV_NAME=$2				#参数2：设备在/dev下的名字
FS_TYPE=vfat			#设备的文件系统类型

if [ $ACTION == "add" ] ; then
#设备插入
#	echo "=======add==========" >> /media/udev.txt
	/bin/mkdir -p $MNT_PATH$DEV_PRE-$DEV_NAME
	#/bin/mount -t $FS_TYPE /dev/$DEV_NAME $MNT_PATH$DEV_PRE-$DEV_NAME
	/bin/mount  /dev/$DEV_NAME $MNT_PATH$DEV_PRE-$DEV_NAME
	if [[ $? -ne 0 ]] ; then #mount失败则清除刚刚建立的目录
#		echo "mount $DEV_NAME fialed" >> /media/udev.txt
		/bin/rm -rf $MNT_PATH$DEV_PRE-$DEV_NAME	
	fi
elif [ $ACTION == "remove" ] ; then 
#	echo "=======remove==========" >> /media/udev.txt
#设备移除
	if [[ -e  $MNT_PATH$DEV_PRE-$DEV_NAME ]] ; then 
		/bin/umount -l  $MNT_PATH$DEV_PRE-$DEV_NAME
		/bin/rmdir  $MNT_PATH$DEV_PRE-$DEV_NAME
	fi
fi

exit 0

