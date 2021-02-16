#!/usr/bin/perl -w

$TARGET_DIR = "../../android/res/";

$INKSCAPE = "inkscape -o";
#$INKSCAPE = "/c/Progra~1/inkscape/inkscape.exe -e";

sub do_convert($$);

#                      dpi: 120       160       240        320         480            640
my %ic_actions_sizes  = (ldpi=>24, mdpi=>32, hdpi=>48, xhdpi=>64, xxhdpi=>96);  # , xxxhdpi=>128);
my %ic_tabs_sizes     = (ldpi=>30, mdpi=>40, hdpi=>60, xhdpi=>80, xxhdpi=>120); # , xxxhdpi=>160);
my %ic_menu_sizes     = (ldpi=>36, mdpi=>48, hdpi=>72, xhdpi=>96, xxhdpi=>144); # , xxxhdpi=>192);
my %ic_launcher_sizes = (ldpi=>36, mdpi=>48, hdpi=>72, xhdpi=>96, xxhdpi=>144); # , xxxhdpi=>192);
my %ic_bigicons_sizes = (ldpi=>36, mdpi=>48, hdpi=>72, xhdpi=>96, xxhdpi=>144); # , xxxhdpi=>192);

my %ic_actions_list=(
	'../google-drive-logo/drive-icon-48x48.svg' => 'google_drive.png',

	'cr3_browser_back-48x48-src.svg' => 'cr3_browser_back.png',
	'cr3_browser_book-48x48-src.svg' => 'cr3_browser_book.png',
	'cr3_browser_book_chm-48x48-src.svg' => 'cr3_browser_book_chm.png',
	'cr3_browser_book_doc-48x48-src.svg' => 'cr3_browser_book_doc.png',
	'cr3_browser_book_epub-48x48-src.svg' => 'cr3_browser_book_epub.png',
	'cr3_browser_book_fb2-48x48-src.svg' => 'cr3_browser_book_fb2.png',
	'cr3_browser_book_fb3-48x48-src.svg' => 'cr3_browser_book_fb3.png',
	'cr3_browser_book_html-48x48-src.svg' => 'cr3_browser_book_html.png',
	'cr3_browser_book_pdb-48x48-src.svg' => 'cr3_browser_book_pdb.png',
	'cr3_browser_book_rtf-48x48-src.svg' => 'cr3_browser_book_rtf.png',
	'cr3_browser_book_txt-48x48-src.svg' => 'cr3_browser_book_txt.png',
	'cr3_browser_book_odt-48x48-src.svg' => 'cr3_browser_book_odt.png',
	'cr3_browser_find-48x48-src.svg' => 'cr3_browser_find.png',
	'cr3_browser_folder-48x48-src.svg' => 'cr3_browser_folder.png',
	'cr3_browser_folder_authors-48x48-src.svg' => 'cr3_browser_folder_authors.png',
	'cr3_browser_folder_current_book-48x48-src.svg' => 'cr3_browser_folder_current_book.png',
	'cr3_browser_folder_opds-48x48-src.svg' => 'cr3_browser_folder_opds.png',
	'cr3_browser_folder_opds_add-48x48-src.svg' => 'cr3_browser_folder_opds_add.png',
	'cr3_browser_folder_recent-48x48-src.svg' => 'cr3_browser_folder_recent.png',
	'cr3_browser_folder_root-48x48-src.svg' => 'cr3_browser_folder_root.png',
	'cr3_browser_folder_zip-48x48-src.svg' => 'cr3_browser_folder_zip.png',
	'cr3_button_add-48x48-src.svg' => 'cr3_button_add.png',
	'cr3_button_book_delete-48x48-src.svg' => 'cr3_button_book_delete.png',
	'cr3_button_bookmarks-48x48-src.svg' => 'cr3_button_bookmarks.png',
	'cr3_button_cancel-48x48-src.svg' => 'cr3_button_cancel.png',
	'cr3_button_folder_go-48x48-src.svg' => 'cr3_button_folder_go.png',
	'cr3_button_more-48x48-src.svg' => 'cr3_button_more.png',
	'cr3_button_next-48x48-src.svg' => 'cr3_button_next.png',
	'cr3_button_prev-48x48-src.svg' => 'cr3_button_prev.png',
	'cr3_button_ok-48x48-src.svg' => 'cr3_button_ok.png',
	'cr3_button_remove-48x48-src.svg' => 'cr3_button_remove.png',
	'cr3_button_scroll_go-48x48-src.svg' => 'cr3_button_scroll_go.png',
	'cr3_button_tts-48x48-src.svg' => 'cr3_button_tts.png',
	'cr3_find_close-48x48-src.svg' => 'cr3_find_close.png',
	'cr3_option_controls_keys-48x48-src.svg' => 'cr3_option_controls_keys.png',
	'cr3_option_controls_tapzones-48x48-src.svg' => 'cr3_option_controls_tapzones.png',
	'cr3_option_font_face-48x48-src.svg' => 'cr3_option_font_face.png',
	'cr3_option_font_gamma-48x48-src.svg' => 'cr3_option_font_gamma.png',
	'cr3_option_font_size-48x48-src.svg' => 'cr3_option_font_size.png',
	'cr3_option_line_spacing-48x48-src.svg' => 'cr3_option_line_spacing.png',
	'cr3_option_page_orientation_landscape-48x48-src.svg' => 'cr3_option_page_orientation_landscape.png',
	'cr3_option_text_align-48x48-src.svg' => 'cr3_option_text_align.png',
	'cr3_option_text_antialias-48x48-src.svg' => 'cr3_option_text_antialias.png',
	'cr3_option_text_bold-48x48-src.svg' => 'cr3_option_text_bold.png',
	'cr3_option_text_hinting-48x48-src.svg' => 'cr3_option_text_hinting.png',
	'cr3_option_text_hyphenation-48x48-src.svg' => 'cr3_option_text_hyphenation.png',
	'cr3_option_text_indent-48x48-src.svg' => 'cr3_option_text_indent.png',
	'cr3_option_text_italic-48x48-src.svg' => 'cr3_option_text_italic.png',
	'cr3_option_text_kerning-48x48-src.svg' => 'cr3_option_text_kerning.png',
	'cr3_option_text_ligatures-48x48-src.svg' => 'cr3_option_text_ligatures.png',
	'cr3_option_text_multilang-48x48-src.svg' => 'cr3_option_text_multilang.png',
	'cr3_option_text_other-48x48-src.svg' => 'cr3_option_text_other.png',
	'cr3_option_text_superscript-48x48-src.svg' => 'cr3_option_text_superscript.png',
	'cr3_option_text_underline-48x48-src.svg' => 'cr3_option_text_underline.png',
	'cr3_option_view_mode_scroll-48x48-src.svg' => 'cr3_option_view_mode_scroll.png',
	'cr3_option_fullscreen-48x48-src.svg' => 'cr3_option_fullscreen.png',
	'cr3_option_font_color-48x48-src.svg' => 'cr3_option_font_color.png',
	'cr3_option_text_margin_bottom-48x48-src.svg' => 'cr3_option_text_margin_bottom.png',
	'cr3_option_text_margin_left-48x48-src.svg' => 'cr3_option_text_margin_left.png',
	'cr3_option_text_margin_top-48x48-src.svg' => 'cr3_option_text_margin_top.png',
	'cr3_option_text_margin_right-48x48-src.svg' => 'cr3_option_text_margin_right.png',
	'cr3_option_text_margins-48x48-src.svg' => 'cr3_option_text_margins.png',
	'cr3_option_text_width-48x48-src.svg' => 'cr3_option_text_width.png',
	'cr3_option_pages_two-48x48-src.svg' => 'cr3_option_pages_two.png',
	'cr3_button_book_open-48x48-src.svg' => 'cr3_button_book_open.png',
	'cr3_button_go_page-48x48-src.svg' => 'cr3_button_go_page.png',
	'cr3_button_go_percent-48x48-src.svg' => 'cr3_button_go_percent.png',
	'cr3_btn_books_swap-48x48-src.svg' => 'cr3_btn_books_swap.png',
	'cr3_viewer_bookmarks-48x48-src.svg' => 'cr3_viewer_bookmarks.png',
	'cr3_viewer_toc-48x48-src.svg' => 'cr3_viewer_toc.png',
	'cr3_button_info-48x48-src.svg' => 'cr3_button_info.png',
	'cr3_button_log-48x48-src.svg' => 'cr3_button_log.png',
	'cr3_button_light-48x48-src.svg' => 'cr3_button_light.png',
	'cr3_button_inc-48x48-src.svg' => 'cr3_button_inc.png',
	'cr3_button_dec-48x48-src.svg' => 'cr3_button_dec.png'
);

my %ic_tabs_list=(
	'cr3_tab_clouds-48x48-src.svg' => 'cr3_tab_clouds.png',
);

my %ic_menu_list=(
);

my %ic_bigicons_list=(
);

do_convert(\%ic_actions_list,  \%ic_actions_sizes);
do_convert(\%ic_tabs_list,     \%ic_tabs_sizes);
do_convert(\%ic_menu_list,     \%ic_menu_sizes);
do_convert(\%ic_bigicons_list, \%ic_bigicons_sizes);

1;

# functions

sub do_convert($$)
{
	my ($src_listref, $src_sizesref) = @_;
	my %src_list = %$src_listref;
	my %src_sizes = %$src_sizesref;

	my ($srcfile, $dstfile);
	my ($dpi, $size);
	my $folder;
	my $resfile;
	my $cmd;
	my $ret;
	my ($srcmtime, $resmtime);

	while (($srcfile, $dstfile) = each(%src_list))
	{
		if (-f $srcfile)
		{
			(undef,undef,undef,undef,undef,undef,undef,undef,undef,$srcmtime,undef,undef,undef) = stat($srcfile);
			while (($dpi, $size) = each(%src_sizes))
			{
				$folder = "${TARGET_DIR}/drawable-${dpi}/";
				if (-d $folder)
				{
					$resfile = "${folder}/${dstfile}";
					$resmtime = 0;
					if (-f $resfile)
					{
						(undef,undef,undef,undef,undef,undef,undef,undef,undef,$resmtime,undef,undef,undef) = stat($resfile);
					}
					if ($srcmtime > $resmtime)
					{
						$cmd = "${INKSCAPE} ${resfile} -w ${size} -h ${size} ${srcfile}";
						print "$cmd\n";
						$ret = system($cmd);
						print "Failed!\n" if $ret != 0;
					}
					else
					{
						print "File \"${srcfile}\" is not newer than \"${resfile}\", skiping.\n";
					}
				}
			}
		}
	}
}
