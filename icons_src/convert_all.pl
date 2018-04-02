#!/usr/bin/perl -w

$TARGET_DIR = "../android/res/";

#                      dpi: 120       160       240        320         480            640
my %ic_actions_sizes  = (ldpi=>24, mdpi=>32, hdpi=>48, xhdpi=>64, xxhdpi=>96,  xxxhdpi=>128);
my %ic_menu_sizes     = (ldpi=>36, mdpi=>48, hdpi=>72, xhdpi=>96, xxhdpi=>144, xxxhdpi=>192);
my %ic_launcher_sizes = (ldpi=>36, mdpi=>48, hdpi=>72, xhdpi=>96, xxhdpi=>144, xxxhdpi=>192);
my %ic_bigicons_sizes = (ldpi=>36, mdpi=>48, hdpi=>72, xhdpi=>96, xxhdpi=>144, xxxhdpi=>192);

my %ic_actions_list=(
	'cr3_button_prev_hc-48x48-src.svg' => 'cr3_button_prev_hc.png',
	'cr3_button_next_hc-48x48-src.svg' => 'cr3_button_next_hc.png',
	'cr3_viewer_toc_hc-256x256-src.svg' => 'cr3_viewer_toc_hc.png',
	'cr3_viewer_find_hc-48x48-src.svg' => 'cr3_viewer_find_hc.png',
	'cr3_viewer_settings_hc-256x256-src.svg' => 'cr3_viewer_settings_hc.png',
	'cr3_button_bookmarks_hc-48x48-src.svg' => 'cr3_button_bookmarks_hc.png',
	'cr3_browser_folder_root_hc-48x48-src.svg' => 'cr3_browser_folder_root_hc.png',
	'cr3_option_night_hc-48x48-src.svg' => 'cr3_option_night_hc.png',
	'cr3_option_touch_hc-256x256-src.svg' => 'cr3_option_touch_hc.png',
	'cr3_button_go_page_hc-256x256-src.svg' => 'cr3_button_go_page_hc.png',
	'cr3_button_go_percent_hc-256x256-src.svg' => 'cr3_button_go_percent_hc.png',
	'cr3_browser_folder_hc-48x48-src.svg' => 'cr3_browser_folder_hc.png',
	'cr3_button_tts_hc-48x48-src.svg' => 'cr3_button_tts_hc.png',
	'cr3_browser_folder_recent_hc-48x48-src.svg' => 'cr3_browser_folder_recent_hc.png',
	'cr3_btn_books_swap_hc-48x48-src.svg' => 'cr3_btn_books_swap_hc.png',
	'cr3_button_scroll_go_hc-48x48-src.svg' => 'cr3_button_scroll_go_hc.png',
	'cr3_logo_button_hc-48x48-src.svg' => 'cr3_logo_button_hc.png',
	'cr3_button_book_open_hc-48x48-src.svg' => 'cr3_button_book_open_hc.png',
	'cr3_viewer_exit_hc-48x48-src.svg' => 'cr3_viewer_exit_hc.png',
	'cr3_browser_folder_current_book_hc-48x48-src.svg' => 'cr3_browser_folder_current_book_hc.png',
	'cr3_browser_folder_opds_hc-48x48-src.svg' => 'cr3_browser_folder_opds_hc.png',
	'cr3_browser_folder_opds_add_hc-48x48-src.svg' => 'cr3_browser_folder_opds_add_hc.png',
	'cr3_button_folder_go_hc-48x48-src.svg' => 'cr3_button_folder_go_hc.png',
	'cr3_option_font_face_hc-48x48-src.svg' => 'cr3_option_font_face_hc.png',
	'cr3_option_font_size_hc-48x48-src.svg' => 'cr3_option_font_size_hc.png',
	'cr3_option_text_bold_hc-48x48-src.svg' => 'cr3_option_text_bold_hc.png',
	'cr3_option_text_italic_hc-48x48-src.svg' => 'cr3_option_text_italic_hc.png',
	'cr3_option_text_underline_hc-48x48-src.svg' => 'cr3_option_text_underline_hc.png',
	'cr3_option_text_superscript_hc-48x48-src.svg' => 'cr3_option_text_superscript_hc.png',
	'cr3_option_text_antialias_hc-48x48-src.svg' => 'cr3_option_text_antialias_hc.png',
	'cr3_option_line_spacing_hc-48x48-src.svg' => 'cr3_option_line_spacing_hc.png',
	'cr3_option_text_hyphenation_hc-48x48-src.svg' => 'cr3_option_text_hyphenation_hc.png',
	'cr3_option_text_floating_punct_hc-48x48-src.svg' => 'cr3_option_text_floating_punct_hc.png',
	'cr3_option_text_kerning_hc-48x48-src.svg' => 'cr3_option_text_kerning_hc.png',
	'cr3_option_images_hc-48x48-src.svg' => 'cr3_option_images_hc.png',
	'cr3_option_font_gamma_hc-48x48-src.svg' => 'cr3_option_font_gamma_hc.png',
	'cr3_option_text_width_hc-48x48-src.svg' => 'cr3_option_text_width_hc.png',
	'cr3_option_fullscreen_hc-48x48-src.svg' => 'cr3_option_fullscreen_hc.png',
	'cr3_option_view_mode_scroll_hc-48x48-src.svg' => 'cr3_option_view_mode_scroll_hc.png',
	'cr3_option_page_orientation_landscape_hc-48x48-src.svg' => 'cr3_option_page_orientation_landscape_hc.png',
	'cr3_option_pages_two_hc-48x48-src.svg' => 'cr3_option_pages_two_hc.png',
	'cr3_option_text_margin_left_hc-48x48-src.svg' => 'cr3_option_text_margin_left_hc.png',
	'cr3_option_text_margin_right_hc-48x48-src.svg' => 'cr3_option_text_margin_right_hc.png',
	'cr3_option_text_margin_top_hc-48x48-src.svg' => 'cr3_option_text_margin_top_hc.png',
	'cr3_option_text_margin_bottom_hc-48x48-src.svg' => 'cr3_option_text_margin_bottom_hc.png',
	'cr3_option_text_align_hc-48x48-src.svg' => 'cr3_option_text_align_hc.png',
	'cr3_option_text_indent_hc-48x48-src.svg' => 'cr3_option_text_indent_hc.png',
	'cr3_option_controls_keys_hc-48x48-src.svg' => 'cr3_option_controls_keys_hc.png',
	'cr3_option_controls_tapzones_hc-48x48-src.svg' => 'cr3_option_controls_tapzones_hc.png',
	'cr3_option_other_hc-48x48-src.svg' => 'cr3_option_other_hc.png',
	'cr3_browser_folder_authors_hc-48x48-src.svg' => 'cr3_browser_folder_authors_hc.png',
	'cr3_browser_folder_zip_hc-48x48-src.svg' => 'cr3_browser_folder_zip_hc.png',
	'cr3_browser_find_hc-48x48-src.svg' => 'cr3_browser_find_hc.png',
	'cr3_browser_back_hc-48x48-src.svg' => 'cr3_browser_back_hc.png',
	'cr3_button_add_hc-48x48-src.svg' => 'cr3_button_add_hc.png',
	'cr3_button_ok_hc-48x48-src.svg' => 'cr3_button_ok_hc.png',
	'cr3_button_cancel_hc-48x48-src.svg' => 'cr3_button_cancel_hc.png'
);

my %ic_menu_list=(
);

my %ic_launcher_list=(
#	'cr3_logo_hc-256x256-src.svg' => 'cr3_logo_hc.png'
);

my %ic_bigicons_list=(
	'folder_big_hc-48x48-src.svg' => 'folder_big_hc.png',
	'folder_big_bookmark_hc-48x48-src.svg' => 'folder_big_bookmark_hc.png',
	'media_flash_microsd-48x48-src.svg' => 'media_flash_microsd.png'
);

my ($srcfile, $dstfile);
my ($dpi, $size);
my $folder;
my $resfile;
my $cmd;
my $ret;

# action icons
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

# menu icons
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

# launcher icons
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

# bigicons
while (($srcfile, $dstfile) = each(%ic_bigicons_list))
{
	while (($dpi, $size) = each(%ic_bigicons_sizes))
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
