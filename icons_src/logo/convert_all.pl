#!/usr/bin/perl -w

# Convert launcher icons with different sizes and populate in appropriate directories.

$TARGET_DIR = "../../android/res/";

#                      dpi: 120       160       240         320         480           640
#    factor:             0.75         1.0       1.5         2.0         3.0           4.0
my %ic_legacy_sizes   = (ldpi=>36, mdpi=>48,  hdpi=>72,  xhdpi=>96,  xxhdpi=>144, xxxhdpi=>192);
my %ic_ad_full_sizes  = (ldpi=>81, mdpi=>108, hdpi=>162, xhdpi=>216, xxhdpi=>324, xxxhdpi=>432);
#my %ic_ad_inner_sizes = (ldpi=>54, mdpi=>72,  hdpi=>108, xhdpi=>144, xxhdpi=>216, xxxhdpi=>288);
my %ic_ad_inner_sizes = (ldpi=>50,  mdpi=>66, hdpi=>99,  xhdpi=>132, xxhdpi=>198, xxxhdpi=>264);

my %ic_legacy_list=(
	'cr3_logo-180x180-store.png' => 'cr3_logo.png'
);

my %ic_ad_foreground_list=(
	'cr3_logo-inner-348x348-unsharp.png' => 'cr3_logo_foreground.png'
);

my %ic_ad_foreground_background_list=(
	'cr3_logo-inner-348x348-unsharp.png' => 'cr3_logo_background-432x432.png'
);

my ($srcfile, $dstfile);
my ($dst_fore_file, $dst_back_file);
my ($dpi, $size, $innsize);
my ($bk_srcfile, $bk_dstfile, $bk_resfile);

my $folder;
my $resfile;
my $cmd;
my $ret;

# Legacy Launcher Icons
while (($srcfile, $dstfile) = each(%ic_legacy_list))
{
	while (($dpi, $size) = each(%ic_legacy_sizes))
	{
		$folder = "${TARGET_DIR}/mipmap-${dpi}/";
		if (-d $folder)
		{
			$resfile = "${folder}/${dstfile}";
			$cmd = "magick ${srcfile} -resize ${size}x${size} ${resfile}";
			print "$cmd\n";
			$ret = system($cmd);
			print "Failed!\n" if $ret != 0;
		}
	}
}

# Adaptive Launcher Icons
while (($srcfile, $dstfile) = each(%ic_ad_foreground_list))
{
	#print "dst_fore_file=${dst_fore_file}\n";
	while (($dpi, $size) = each(%ic_ad_full_sizes))
	{
		$folder = "${TARGET_DIR}/mipmap-${dpi}/";
		$innsize = $ic_ad_inner_sizes{$dpi};
		if (-d $folder)
		{
			$resfile = "${folder}/${dstfile}";

			#if (0)
			#{
			# create empty transparent file
			$cmd = "magick convert -size ${size}x${size} canvas:none ${resfile}";
			print "${cmd}\n";
			$ret = system($cmd);
			if ($ret != 0)
			{
				print "Failed!\n";
				last;
			}

			# resize inner image
			$cmd = "magick ${srcfile} -resize ${innsize}x${innsize} _inner_${innsize}x${innsize}_tmp.png";
			print "${cmd}\n";
			$ret = system($cmd);
			if ($ret != 0)
			{
				print "Failed!\n";
				last;
			}

			# composite
			$cmd = "magick composite -gravity center _inner_${innsize}x${innsize}_tmp.png ${resfile} ${resfile}";
			print "${cmd}\n";
			$ret = system($cmd);
			if ($ret != 0)
			{
				print "Failed!\n";
				last;
			}

			$cmd = "rm -f _inner_${innsize}x${innsize}_tmp.png";
			print "${cmd}\n";
			$ret = system($cmd);
			if ($ret != 0)
			{
				print "Failed!\n";
				last;
			}
			#}
			
			# background
			$bk_srcfile = $ic_ad_foreground_background_list{$srcfile};
			$bk_dstfile = $dstfile;
			$bk_dstfile =~ s/_foreground/_background/;
			$bk_resfile = "${folder}/${bk_dstfile}";
			$cmd = "magick ${bk_srcfile} -resize ${size}x${size} ${bk_resfile}";
			print "${cmd}\n";
			$ret = system($cmd);
			if ($ret != 0)
			{
				print "Failed!\n";
				last;
			}
		}
	}
}
