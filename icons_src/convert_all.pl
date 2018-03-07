#!/usr/bin/perl -w

$TARGET_DIR = "../android/res/";

#                      dpi: 120       160       240        320         480            640
my %ic_actions_sizes  = (ldpi=>24, mdpi=>32, hdpi=>48, xhdpi=>64, xxhdpi=>96,  xxxhdpi=>128);
my %ic_menu_sizes     = (ldpi=>36, mdpi=>48, hdpi=>72, xhdpi=>96, xxhdpi=>144, xxxhdpi=>192);
my %ic_launcher_sizes = (ldpi=>36, mdpi=>48, hdpi=>72, xhdpi=>96, xxhdpi=>144, xxxhdpi=>192);

my %ic_actions_list=(
	'cr3_button_prev_hc-256x256-src.svg' => 'cr3_button_prev_hc.png',
	'cr3_button_next_hc-256x256-src.svg' => 'cr3_button_next_hc.png',
	'cr3_viewer_toc_hc-256x256-src.svg' => 'cr3_viewer_toc_hc.png',
	'cr3_viewer_find_hc-256x256-src.svg' => 'cr3_viewer_find_hc.png',
	'cr3_viewer_settings_hc-256x256-src.svg' => 'cr3_viewer_settings_hc.png',
	'cr3_button_bookmarks_hc-256x256-src.svg' => 'cr3_button_bookmarks_hc.png',
	'cr3_browser_folder_root_hc-256x256-src.svg' => 'cr3_browser_folder_root_hc.png',
	'cr3_option_night_hc-256x256-src.svg' => 'cr3_option_night_hc.png',
	'cr3_option_touch_hc-256x256-src.svg' => 'cr3_option_touch_hc.png',
	'cr3_button_go_page_hc-256x256-src.svg' => 'cr3_button_go_page_hc.png',
	'cr3_button_go_percent_hc-256x256-src.svg' => 'cr3_button_go_percent_hc.png',
	'cr3_browser_folder_hc-48x48-src.svg' => 'cr3_browser_folder_hc.png',
	'cr3_button_tts_hc-48x48-src.svg' => 'cr3_button_tts_hc.png',
	'cr3_browser_folder_recent_hc-48x48-src.svg' => 'cr3_browser_folder_recent_hc.png',
	'cr3_btn_books_swap_hc-48x48-src.svg' => 'cr3_btn_books_swap_hc.png',
	'cr3_button_scroll_go_hc-48x48-src.svg' => 'cr3_button_scroll_go_hc.png',
	'cr3_logo_hc-48x48-src.svg' => 'cr3_logo_button_hc.png'
	'cr3_button_book_open_hc-256x256-src.svg' => 'cr3_button_book_open_hc.png',
);

my %ic_menu_list=(
);

my %ic_launcher_list=(
#	'cr3_logo_hc-256x256-src.svg' => 'cr3_logo_hc.png'
);

my ($srcfile, $dstfile);
my ($dpi, $size);
my $folder;
my $resfile;
my $cmd;
my $ret;
while (($srcfile, $dstfile) = each(%ic_actions_list))
{
	while (($dpi, $size) = each(%ic_actions_sizes))
	{
		$folder = "${TARGET_DIR}/drawable-${dpi}/";
		if (-d $folder)
		{
			$resfile = "${folder}/${dstfile}";
			$cmd = "inkscape -z -e ${resfile} -w ${size} -h ${size} ${srcfile}";
			print "$cmd\n";
			$ret = system($cmd);
			print "Failed!\n" if $ret != 0;
		}
	}
}

while (($srcfile, $dstfile) = each(%ic_menu_list))
{
	while (($dpi, $size) = each(%ic_menu_sizes))
	{
		$folder = "${TARGET_DIR}/drawable-${dpi}/";
		if (-d $folder)
		{
			$resfile = "${folder}/${dstfile}";
			$cmd = "inkscape -z -e ${resfile} -w ${size} -h ${size} ${srcfile}";
			print "$cmd\n";
			$ret = system($cmd);
			print "Failed!\n" if $ret != 0;
		}
	}
}

while (($srcfile, $dstfile) = each(%ic_launcher_list))
{
	while (($dpi, $size) = each(%ic_launcher_sizes))
	{
		$folder = "${TARGET_DIR}/drawable-${dpi}/";
		if (-d $folder)
		{
			$resfile = "${folder}/${dstfile}";
			$cmd = "inkscape -z -e ${resfile} -w ${size} -h ${size} ${srcfile}";
			print "$cmd\n";
			$ret = system($cmd);
			print "Failed!\n" if $ret != 0;
		}
	}
}
