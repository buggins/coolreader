/*
 * Additional CCRTable methods needed for MathML support.
 * #include'd by lvrend.cpp, externalized to keep lvrend.cpp table
 * code readable
 *
 * This is used to render <msub> <msup> <msubsup> <mmultiscripts>
 * <munder> <mover> <munderover> with the help of our table
 * rendering code by handling them as inline-table, with a few
 * sizing, positionning and baseline tweaks.
 * This file plugs these additional tweaks into our CCRTable code.
 */

// Called after walking DOM elements, and before building
// the internal table grid data structures
void CCRTable::MathML_checkAndTweakTableElement() {
    if ( !enhanced_rendering )
        return;
    lUInt16 tableElementId = elem->getNodeId();

    // msub/msup/msubsup/mmultiscripts, or munder/mover/munderover that should be rendered as msub/msup/msupsup
    if ( (tableElementId >= el_msub && tableElementId <= el_mmultiscripts) ||
         (tableElementId >= el_munder && tableElementId <= el_munderover && elem->hasAttribute(attr_Msubsup) ) ) {

        // We get a single row with all sub-elements as table cells.
        // We want to tweak the table layout (as after LookupElem(), and
        // ready for PlaceCells() to do its work) so the table renders
        // as expected for these MathML elements: as multiple rows
        if ( !rows.length() ) // no sub element added
            return;
        CCRTableRow * row1 = rows[0];
        if ( row1->cells.length() < 2 ) // only a base, no sub/sup
            return;

        // Remember what kind of element we're working with, as We may tweak it here and elsewhere
        mathml_tweaked_element_name_id = tableElementId;
        if (tableElementId >= el_munder && tableElementId <= el_munderover) {
            // munder/mover/munderover that should be rendered as msub/sup/supsup:
            // set it to how it should be rendered
            switch (tableElementId) {
                case el_munder:     mathml_tweaked_element_name_id = el_msub;    break;
                case el_mover:      mathml_tweaked_element_name_id = el_msup;    break;
                case el_munderover: mathml_tweaked_element_name_id = el_msubsup; break;
                default: break;
            }
        }

        // Add 2 other rows
        CCRTableRow * row2 = new CCRTableRow;
        rows.add( row2 );
        CCRTableRow * row3 = new CCRTableRow;
        rows.add( row3 );

        // We want to move some of the cells to the other rows,
        // (and for <mmultiscripts> move those after <mprescripts/>
        // at start of rows)
        // We want:
        // In the 1st row: the base (rowspan=3) and the superscripts
        // In the 2nd row: an empty cell acting as a strut to keep
        // superscript and subscripts from touching
        // In the 3rd row: subscripts
        row1->cells[0]->rowspan = 3;
        row1->cells[0]->valign = 1; // top, to not be baseline so to not be adjusted by renderCells()
        CCRTableCell * vertical_strut = new CCRTableCell;
        row2->cells.add(vertical_strut); // only cell present in row2
        vertical_strut->row = row2;

        bool next_is_sup = mathml_tweaked_element_name_id == el_msup; // (so also with el_mover)
        int insert_at = -1; // updated when <mprescripts> met
        int i = 1;
        while (i < row1->cells.length()) {
            row1->cells[i]->valign = 0; // baseline
            if (row1->cells[i]->elem && row1->cells[i]->elem->getNodeId() == el_mprescripts) {
                // <mprescripts> is an empty element
                if (next_is_sup) {
                    i++; // keep it in row1 as a filler for the missing cell at top right
                }
                else {
                    delete row1->cells.remove(i); // remove it
                }
                insert_at = 0; // next cells will be moved at start of rows
                next_is_sup = false; // next is a sub
                continue;
            }
            if (next_is_sup) {
                if (insert_at >= 0) {
                    row1->cells.insert(insert_at, row1->cells.remove(i));
                    i++;
                    // next will be sub that should be added in a new column on the right of this one
                    insert_at++;
                }
                else {
                    i++; // let this cell be where it was in row1
                }
            }
            else {
                // move subs to row3
                CCRTableCell * cell = row1->cells.remove(i);
                row3->cells.insert(insert_at, cell ); // insert() does append when index is -1
                cell->row = row3;
            }
            next_is_sup = !next_is_sup;
            if ( next_is_sup && insert_at >= 0 && i >= row1->cells.length() ) {
                // No even number of elements after <prescripts/>:
                // add a missing cell to not mess table layout
                CCRTableCell * cell = new CCRTableCell;
                cell->row = row1;
                row1->cells.insert(insert_at, cell);
                break;
            }
        }
        rows_rendering_reordered = true;
    }

    // Normal munder/mover/munderover
    else if ( tableElementId >= el_munder && tableElementId <= el_munderover ) {
        if ( !rows.length() ) // no sub element added
            return;
        CCRTableRow * row1 = rows[0];
        if ( row1->cells.length() < 2 ) // only a base, no under/over
            return;

        // Remember what kind of element we're working with, as We may tweak it here and elsewhere
        mathml_tweaked_element_name_id = tableElementId;

        // Add a second row
        CCRTableRow * row2 = new CCRTableRow;
        rows.add( row2 );
        if ( tableElementId == el_mover ) {
            // Move first child (base) in second row
            CCRTableCell * cell = row1->cells.remove(0);
            row2->cells.add(cell);
            cell->row = row2;
        }
        else if ( tableElementId == el_munder ) {
            // Move second child (under) in second row
            CCRTableCell * cell = row1->cells.remove(1);
            row2->cells.add(cell);
            cell->row = row2;
        }
        else { // munderover
            // Add a third row
            CCRTableRow * row3 = new CCRTableRow;
            rows.add( row3 );
            // Move second child (under) in third row
            CCRTableCell * cell = row1->cells.remove(1);
            row3->cells.add(cell);
            cell->row = row3;
            // Move first child (base) in second row
            cell = row1->cells.remove(0);
            row2->cells.add(cell);
            cell->row = row2;
            // So, first row contains the original third child (over)
        }
        rows_rendering_reordered = true;
    }

    // msqrt / mroot
    else if ( tableElementId == el_msqrt || tableElementId == el_mroot ) {
        // msqrt and mroot are a single row with 2 cells. We want the last
        // cell (the root symbol with mroot degree) to come first
        if ( rows.length() != 1 ) // no sub element added
            return;
        CCRTableRow * row1 = rows[0];
        if ( row1->cells.length() != 2 ) // not the 2 expected components
            return;
        // Remember what kind of element we're working with, as we may tweak it here and elsewhere
        mathml_tweaked_element_name_id = tableElementId;
        // Swap cells
        row1->cells.move(0, 1);
        // Nothing else to do
    }

    else if ( tableElementId == el_mfrac ) {
        // No re-ordering, we'll just want to ensure proper numerator/denominator vertical positions
        if ( rows.length() != 2 ) // no sub element added
            return;
        if ( rows[0]->cells.length() != 1 || rows[1]->cells.length() != 1 )
            return;
        // Set cells valign=top, to ease our computations by not interfering with
        // row baselines (we'll handle the first row baseline specifically)
        rows[0]->cells[0]->valign = 1;
        rows[1]->cells[0]->valign = 1;
        mathml_tweaked_element_name_id = tableElementId;
    }
    else if ( tableElementId == el_mtable ) {
        // No real tweak needed, but we'll want to look for <mo> single child of <mtd>
        // that, if stretchy, should stretch to occupy the whole cell
        mathml_tweaked_element_name_id = tableElementId;
    }

    else {
        // Not a MathML tag, or not a MathML tag that needs tweaking
        return;
    }
}

// Called after individual cell dimensions measurement, before they
// are accounted to make rows height and y, and table height.
// We adjust each cell height and baseline according to the MathML
// specs, using the ink box and the font math metrics.
void CCRTable::MathML_fixupTableLayout() {

    // msub/msup/msubsup/mmultiscripts, or munder/mover/munderover that should be rendered as msub/msup/msupsup
    if ( mathml_tweaked_element_name_id >= el_msub && mathml_tweaked_element_name_id <= el_mmultiscripts ) {
        // See MathJax's ts/output/chtml/Wrappers/mmultiscripts.ts,
        // ts/output/common/Wrappers/mmultiscripts.ts and msubsup.ts
        // for some more complex computations.
        // We use some simpler rules...
        int base_idx = -1;
        CCRTableCell * base_cell = NULL;;
        for (int j=0; j<rows[0]->cells.length(); j++) {
            if ( rows[0]->cells[j]->rowspan > 1) {
                base_idx = j;
                base_cell = rows[0]->cells[j];
                // switch valign to baseline (was set to 'top' to not
                // intervene in first row baseline), so it won't be
                // shifted when aligning cells in row
                base_cell->valign = 0;
                base_cell->adjusted_baseline = base_cell->baseline;
                break;
            }
        }

        if ( base_cell ) {
            // We could get metrics from the font of the table element (<msub>...)
            // or from the base element (which may have a different font,
            // i.e. when it's a <mo> with "largeop").
            // Not sure which is the best.
            // LVFontRef base_font = elem->getFont();
            LVFontRef base_font = base_cell->elem->getFont();

            // rows[0] contains our superscripts, we know its height and baseline (without base)
            // rows[2] contains our subscripts, we know its height and baseline
            // rows[1] contains a strut whose height is a minimal height
            // (We name the variables upper_*/lower_*, because sup_*/sub_* is prone to typo)
            int base_h = base_cell->height;
            int base_b = base_cell->baseline; // original baseline
            int base_ink_ascent = 0;
            int base_ink_descent = 0;
            lvRect base_ink_offsets;
            if ( getInkOffsets( base_cell->elem, base_ink_offsets, true) ) {
                base_ink_ascent = base_b - base_ink_offsets.top;
                base_ink_descent = base_h - base_ink_offsets.bottom - base_b;
            }

            bool has_upper = false;
            int upper_h = rows[0]->height;
            int upper_b = rows[0]->baseline;
            int upper_ink_ascent = 0;
            int upper_ink_descent = 0;
            if (rows[0]->cells.length() > 1) {
                bool upper_ink_set = false;
                for (int j=0; j<rows[0]->cells.length(); j++) {
                    if (j == base_idx)
                        continue;
                    CCRTableCell * cell = rows[0]->cells[j];
                    if ( !cell->elem )
                        continue;
                    lvRect ink_offsets;
                    if ( getInkOffsets( cell->elem, ink_offsets, true) ) {
                        has_upper = true;
                        // We want the ink ascent and descent related to the row baseline.
                        int cell_shift_down = cell->adjusted_baseline - cell->baseline;
                        int ink_ascent = upper_b - (cell_shift_down + ink_offsets.top);
                        int ink_descent = (cell_shift_down + cell->height - ink_offsets.bottom) - upper_b;
                        if ( !upper_ink_set ) {
                            upper_ink_ascent = ink_ascent;
                            upper_ink_descent = ink_descent;
                            upper_ink_set = true;
                        }
                        else {
                            if ( upper_ink_ascent < ink_ascent )
                                upper_ink_ascent = ink_ascent;
                            if ( upper_ink_descent < ink_descent )
                                upper_ink_descent = ink_descent;
                        }
                    }
                }
            }
            // Note: we got ink offsets like mentionned in the specs. It reads as it should really
            // be the glyphes ink, so 'p' and 'm' as superscript won't get the same ink height,
            // and so might be vertically positionned differently vs. their respective base.
            // But with Firefox, 2 bases, one with 'p' as sup and the other 'm', will get the
            // 'p' and 'm' baselines coincide... This could mean the ink should not be based
            // on each glyph blackbox, but on somehow the font max ascender/descender (which
            // would need some tweaks to lvfntman.cpp DrawTextString() to provide some other
            // value when buf->Draw() on our LVInkMeasurementDrawBuf.

            if ( has_upper ) {
                // https://mathml-refresh.github.io/mathml-core/#base-with-superscript
                // SuperShift (the distance between baselines) is the maximum between:
                // 1. The value SuperscriptShiftUpCramped if the element has a computed math-shift
                // property equal to compact, or SuperscriptShiftUp otherwise.
                int super_shift_1 = elem->hasAttribute(attr_MS) ? // (this is the single use for MS/math-shift)
                    base_font->getExtraMetric(font_metric_math_superscript_shift_up) :
                    base_font->getExtraMetric(font_metric_math_superscript_shift_up_cramped);
                // 2. SuperscriptBottomMin + the ink line-descent of the superscript's margin box
                int super_shift_2 = base_font->getExtraMetric(font_metric_math_superscript_bottom_min) + upper_ink_descent;
                // 3. The ink line-ascent of the base's margin box - SuperscriptBaselineDropMax
                int super_shift_3 = base_ink_ascent - base_font->getExtraMetric(font_metric_math_superscript_baseline_drop_max);
                // Get the largest of them
                int super_shift = super_shift_1 > super_shift_2 ? super_shift_1 : super_shift_2;
                super_shift = super_shift_3 > super_shift ? super_shift_3 : super_shift;

                int cur_upper_b_dy = base_b - upper_b;
                int d_upper_b_dy = cur_upper_b_dy - super_shift;
                // printf("(base b/h=%d/%d sup b/h=%d/%d) super_shift_1=%d super_shift_2=%d super_shift_3=%d => upper_b_dy=%d | cur_upper_b_dy=%d > d_upper_b_dy=%d\n",
                //     base_b, base_h, upper_b, upper_h, super_shift_1, super_shift_2, super_shift_3, super_shift, cur_upper_b_dy, d_upper_b_dy);
                if ( d_upper_b_dy > 0 ) {
                    // base currently too low: shift the row baseline down
                    rows[0]->baseline += d_upper_b_dy;
                    rows[0]->height += d_upper_b_dy;
                    upper_h = rows[0]->height;
                    upper_b = rows[0]->baseline;
                }
                else {
                    // base currently too high: shift its cell baseline down
                    base_cell->adjusted_baseline += -d_upper_b_dy;
                    base_cell->height += -d_upper_b_dy;
                    base_h = base_cell->height;
                    base_b = base_cell->adjusted_baseline;
                }
            }

            int strut_h = rows[1]->height; // 0 at this point

            // Our 3rd row may have been removed if no cell (with <msup>)
            bool has_lower = false;
            int lower_top_y = upper_h + strut_h;
            int lower_h = 0;
            int lower_b = 0;
            int lower_ink_ascent = 0;
            int lower_ink_descent = 0;
            if (rows.length() > 2 && rows[2]->cells.length() > 0) {
                // Subscripts row
                // We won't change its height or baseline. We'll just increase the strut height,
                // or move the base down.
                lower_h = rows[2]->height;
                lower_b = rows[2]->baseline;
                bool lower_ink_set = false;
                for (int j=0; j<rows[2]->cells.length(); j++) {
                    CCRTableCell * cell = rows[2]->cells[j];
                    if ( !cell->elem )
                        continue;
                    lvRect ink_offsets;
                    if ( getInkOffsets( cell->elem, ink_offsets, true) ) {
                        has_lower = true;
                        // We want the ink ascent and descent related to the row baseline.
                        int cell_shift_down = cell->adjusted_baseline - cell->baseline;
                        int ink_ascent = lower_b - (cell_shift_down + ink_offsets.top);
                        int ink_descent = (cell_shift_down + cell->height - ink_offsets.bottom) - lower_b;
                        if ( !lower_ink_set ) {
                            lower_ink_ascent = ink_ascent;
                            lower_ink_descent = ink_descent;
                            lower_ink_set = true;
                        }
                        else {
                            if ( lower_ink_ascent < ink_ascent )
                                lower_ink_ascent = ink_ascent;
                            if ( lower_ink_descent < ink_descent )
                                lower_ink_descent = ink_descent;
                        }
                    }
                }
            }

            if ( has_lower ) {
                // https://mathml-refresh.github.io/mathml-core/#base-with-subscript
                // SubShift (the distance between baselines) is the maximum between:
                // 1. SubscriptShiftDown
                int sub_shift_1 = base_font->getExtraMetric(font_metric_math_subscript_shift_down);
                // 2. The ink line-ascent of the subscript's margin box - SubscriptTopMax
                int sub_shift_2 = lower_ink_ascent - base_font->getExtraMetric(font_metric_math_subscript_top_max);
                // 3. SubscriptBaselineDropMin + the ink line-descent of the base's margin box
                int sub_shift_3 = base_font->getExtraMetric(font_metric_math_subscript_baseline_drop_min) + base_ink_descent;
                // Get the largest of them
                int sub_shift = sub_shift_1 > sub_shift_2 ? sub_shift_1 : sub_shift_2;
                sub_shift = sub_shift_3 > sub_shift ? sub_shift_3 : sub_shift;

                int cur_lower_b_dy = lower_top_y + lower_b - base_b;
                int d_lower_b_dy = sub_shift - cur_lower_b_dy;
                // printf("(base b/h=%d/%d sub b/h=%d/%d) sub_shift_1=%d sub_shift_2=%d sub_shift_3=%d => sub_shift=%d | cur_lower_b_dy=%d > d_lower_b_dy=%d\n",
                //    base_b, base_h, lower_b, lower_h, sub_shift_1, sub_shift_2, sub_shift_3, sub_shift, cur_lower_b_dy, d_lower_b_dy);
                if ( d_lower_b_dy > 0 ) {
                    // shift down
                    rows[1]->height += d_lower_b_dy;
                    strut_h = rows[1]->height;
                }
                else {
                    // base currently too high: shift its cell baseline down
                    base_cell->adjusted_baseline += -d_lower_b_dy;
                    base_cell->height += -d_lower_b_dy;
                    base_h = base_cell->height;
                    base_b = base_cell->adjusted_baseline;
                }
            }

            if ( has_upper && has_lower ) {
                // https://mathml-refresh.github.io/mathml-core/#base-with-subscript-and-superscript
                // If SubSuperGap is not at least SubSuperscriptGapMin... (see url for the details)
                lower_top_y = upper_h + strut_h;
                int upper_gap = base_b - (upper_b + upper_ink_descent);
                int lower_gap = (lower_top_y + lower_b - lower_ink_ascent) - base_b;
                int sub_super_gap = upper_gap + lower_gap;
                int sub_super_gap_min = base_font->getExtraMetric(font_metric_math_sub_superscript_gap_min);
                if ( sub_super_gap < sub_super_gap_min ) {
                    // 1. let delta be... (see url for the details)
                    int super_bottom_max = base_font->getExtraMetric(font_metric_math_superscript_bottom_max_with_subscript);
                    int delta = super_bottom_max - upper_gap;
                    if ( delta > 0 ) {
                        int delta2 = sub_super_gap_min - sub_super_gap;
                        if ( delta2 < delta )
                            delta = delta2;
                        // shift base down
                        base_cell->adjusted_baseline += delta;
                        base_cell->height += delta;
                        base_h = base_cell->height;
                        base_b = base_cell->adjusted_baseline;
                        // printf("sub_super_gap step1 moved by %d\n", delta);
                        // Update above computed values
                        upper_gap = base_b - (upper_b + upper_ink_descent);
                        lower_gap = (lower_top_y + lower_b - lower_ink_ascent) - base_b;
                        sub_super_gap = upper_gap + lower_gap;
                    }
                }
                if ( sub_super_gap < sub_super_gap_min ) {
                    // 2. let delta be... (see url for the details)
                    int delta = sub_super_gap_min - sub_super_gap;
                    // shift lower down
                    rows[1]->height += delta;
                    strut_h = rows[1]->height;
                    // printf("sub_super_gap step2 moved by %d\n", delta);
                }
            }

            rows[0]->baseline = base_cell->adjusted_baseline;

            // printf("base_h=%d upper_h=%d strut_h=%d lower_h=%d\n", base_h, upper_h, strut_h, lower_h);
            int d_h = base_h - (upper_h + strut_h + lower_h);
            if (d_h > 0) {
                // Put the extra height on the strut
                rows[1]->height += d_h;
            }

            if (rows.length() > 2 && rows[2]->cells.length() > 0) {
                // Italic correction
                ldomNode * baseNode = base_cell->elem;
                if ( baseNode->hasAttribute(attr_Memb) ) {
                    ldomNode * moNode = getMathMLCoreEmbelishedOperator(baseNode);
                    if ( moNode ) {
                        LVFontRef font = moNode->getFont();
                        lString32 txt = moNode->getText();
                        int correction;
                        if ( font->getGlyphExtraMetric(glyph_metric_math_italics_correction, txt.lastChar(), correction, true) ) {
                            if ( correction > 0 ) {
                                for (int j=base_idx; j<rows[2]->cells.length(); j++) {
                                    CCRTableCell * cell = rows[2]->cells[j];
                                    if ( cell && cell->elem ) {
                                        RenderRectAccessor fmt( cell->elem );
                                        fmt.setX(fmt.getX() - correction);
                                    }
                                }
                                // Note: with most fonts, this correction looks like not enough...
                                // but with very few fonts (Asana Math, Latin Modern Math), it is just right...
                                // So, we can't correct it with *2 as we'd like to, assume it is some font bug
                                // (not enough is bearable, too much would be bad).
                                // Same below with munder/mover/munderover.
                            }
                        }
                    }
                }
            }
        }
    }

    // Normal munder/mover/munderover
    else if ( mathml_tweaked_element_name_id >= el_munder && mathml_tweaked_element_name_id <= el_munderover ) {
        CCRTableRow * base_row = NULL;
        CCRTableRow * over_row = NULL;
        CCRTableRow * under_row = NULL;
        CCRTableCell * base_cell = NULL;
        CCRTableCell * over_cell = NULL;
        CCRTableCell * under_cell = NULL;
        ldomNode * base_node = NULL;
        ldomNode * over_node = NULL;
        ldomNode * under_node = NULL;
        if ( mathml_tweaked_element_name_id == el_mover ) {
            over_row = rows[0];
            base_row = rows[1];
        }
        else if ( mathml_tweaked_element_name_id == el_munder ) {
            base_row = rows[0];
            under_row = rows[1];
        }
        else { // munderover
            over_row = rows[0];
            base_row = rows[1];
            under_row = rows[2];
        }
        base_cell = base_row->cells[0];
        base_node = base_cell->elem;

        bool over_is_accent = false;
        if ( mathml_tweaked_element_name_id != el_munder ) {
            if ( elem->hasAttribute(attr_Maccent) )
                over_is_accent = true;
            else if ( elem->hasAttribute(attr_accent) && elem->getAttributeValueLC(attr_accent) == U"true" )
                over_is_accent = true;
        }
        bool under_is_accent = false;
        if ( mathml_tweaked_element_name_id != el_mover ) {
            if ( elem->hasAttribute(attr_Maccentunder) )
                under_is_accent = true;
            else if ( elem->hasAttribute(attr_accentunder) && elem->getAttributeValueLC(attr_accentunder) == U"true" )
                under_is_accent = true;
        }

        // See if we need to apply a shift to over & under's x if the base is slanted,
        // and if we should use other metrics if largeop or hstretchy
        int italic_correction_over = 0;
        int italic_correction_under = 0;
        bool is_core_largeop = false;
        bool is_core_hstretchy = false;
        if ( base_node->hasAttribute(attr_Memb) ) {
            ldomNode * mo_node = getMathMLCoreEmbelishedOperator(base_node);
            if ( mo_node ) {
                LVFontRef font = mo_node->getFont();
                lString32 txt = mo_node->getText();
                int italic_correction;
                font->getGlyphExtraMetric(glyph_metric_math_italics_correction, txt.lastChar(), italic_correction, true);
                italic_correction_over = italic_correction / 2;
                italic_correction_under = italic_correction / 2;
                if ( mo_node->hasAttribute(attr_Mlargeop) )
                    is_core_largeop = true;
                if ( mo_node->hasAttribute(attr_Mtransform) && mo_node->getAttributeValue(attr_Mtransform) == U"hstretch" )
                    is_core_hstretchy = true;
            }
        }
        else {
            // Not an operator: but if the base is a single italic char, and over/under are accents,
            // we prefer to have them shifted a bit so they align better with the glyph.
            // (This is not mentionned by the specs, but it looks way better.)
            if ( over_is_accent || under_is_accent ) {
                ldomNode * textNode = base_node->getFirstTextChild();
                if ( textNode && textNode == base_node->getLastTextChild() ) { // single text node
                    lString32 txt = textNode->getText();
                    if ( txt.length() == 1 ) { // single char
                        // Trust the font to detect and return some correction only for italic unicode chars
                        int italic_correction;
                        LVFontRef font = textNode->getParentNode()->getFont();
                        if ( font->getGlyphExtraMetric(glyph_metric_math_italics_correction, txt[0], italic_correction, true) ) {
                            if ( over_is_accent )
                                italic_correction_over = italic_correction / 2;
                            if ( under_is_accent )
                                italic_correction_under = italic_correction / 2;
                        }
                    }
                }
            }
        }

        // Cap the over and under rows & cells height to their ink area
        int over_ink_descent = 0;
        if ( over_row ) {
            over_cell = over_row->cells[0];
            over_node = over_cell->elem;
            if ( over_node ) {
                lvRect over_ink_offsets;
                bool over_has_ink = getInkOffsets( over_node, over_ink_offsets, true);
                RenderRectAccessor over_fmt( over_node );
                if ( italic_correction_over > 0 )
                    over_fmt.setX(over_fmt.getX() + italic_correction_over);
                // Adjust the row and cell height to their ink area
                if ( over_has_ink ) {
                    over_ink_descent = over_cell->height - over_cell->baseline - over_ink_offsets.bottom;
                    over_row->height -= over_ink_offsets.top + over_ink_offsets.bottom;
                    over_cell->height -= over_ink_offsets.top + over_ink_offsets.bottom;
                    over_fmt.setY(over_fmt.getY() - over_ink_offsets.top);
                }
                else {
                    over_row->height = 0;
                    over_cell->height = 0;
                }
            }
        }
        int under_ink_ascent = 0;
        if ( under_row ) {
            under_cell = under_row->cells[0];
            under_node = under_cell->elem;
            if ( under_node ) {
                lvRect under_ink_offsets;
                bool under_has_ink = getInkOffsets( under_node, under_ink_offsets, true);
                RenderRectAccessor under_fmt( under_node );
                if ( italic_correction_under > 0 )
                    under_fmt.setX(under_fmt.getX() - italic_correction_under);
                // Adjust the row and cell height to their ink area
                if ( under_has_ink ) {
                    under_ink_ascent = under_cell->baseline - under_ink_offsets.top;
                    under_row->height -= under_ink_offsets.top + under_ink_offsets.bottom;
                    under_cell->height -= under_ink_offsets.top + under_ink_offsets.bottom;
                    under_fmt.setY(under_fmt.getY() - under_ink_offsets.top);
                }
                else {
                    under_row->height = 0;
                    under_cell->height = 0;
                }
            }
        }

        // Cap the base cell to its ink area, and re-increase it to shift
        // the over and under rows/cells as needed
        lvRect base_ink_offsets;
        RenderRectAccessor base_fmt( base_node );
        bool base_has_ink = getInkOffsets( base_node, base_ink_offsets, true);
        if ( base_has_ink ) {
            base_row->height -= base_ink_offsets.top + base_ink_offsets.bottom;
            base_row->baseline -= base_ink_offsets.top;
            base_cell->height -= base_ink_offsets.top + base_ink_offsets.bottom;
            base_fmt.setY(base_fmt.getY() - base_ink_offsets.top);
        }
        else {
            base_row->height = 0;
            base_cell->height = 0;
        }
        if ( over_node ) {
            // See https://mathml-refresh.github.io/mathml-core/#base-with-overscript
            // Note that this spec shows Undershift/Overshift applied to some kind of under/over "baseline"
            // or "ink baseline", which is not defined anywhere (and can't be the glyph baseline, as an
            // accent would have it way below its glyph.
            // We apply Undershift/Overshift to the bottom of the ink box, which seems to match
            // what Firefox does.
            // This means movers like 'p' 'q' 'r' can have the 'r' lower than the 'p' and 'q', but
            // in some cases, the OT Math font metrics can ensure a minimal gap that hides this.
            int gap = 0;
            if ( over_is_accent ) {
                int min_height = base_node->getFont()->getExtraMetric(font_metric_math_accent_base_height);
                if ( base_row->baseline < min_height ) {
                    gap = min_height - base_row->baseline;
                }
                if (gap == 0) gap = 1; // min 1px added to void glyph merging
            }
            else {
                gap = base_node->getFont()->getExtraMetric(font_metric_math_overbar_vertical_gap);
            }
            if ( is_core_largeop ) {
                int gap1 = base_node->getFont()->getExtraMetric(font_metric_math_upper_limit_baseline_rise_min);
                gap1 = gap1 != 0 ? gap1 - over_ink_descent : gap; // fallback to gap if metric absent
                int gap2 = base_node->getFont()->getExtraMetric(font_metric_math_upper_limit_gap_min);
                gap = gap1 > gap2 ? gap1 : gap2;
            }
            else if ( is_core_hstretchy ) {
                // (Note: with some math fonts, this metric can be larger than what would feel needed, and
                // there can be a huge space above the stretchy glyph...)
                int gap1 = base_node->getFont()->getExtraMetric(font_metric_math_stretch_stack_top_shift_up);
                gap1 = gap1 != 0 ? gap1 - over_ink_descent : gap; // fallback to gap if metric absent
                int gap2 = base_node->getFont()->getExtraMetric(font_metric_math_stretch_stack_gap_below_min);
                gap = gap1 > gap2 ? gap1 : gap2;
            }
            base_row->height += gap;
            base_row->baseline += gap;
            base_cell->height += gap;
            base_fmt.setY(base_fmt.getY() + gap);
            if ( !is_core_largeop && !is_core_hstretchy ) {
                // padding at top of over
                gap = base_node->getFont()->getExtraMetric(font_metric_math_overbar_extra_ascender);
                over_row->height += gap;
                over_cell->height += gap;
                RenderRectAccessor over_fmt( over_node );
                over_fmt.setY(over_fmt.getY() + gap);
            }
        }
        if ( under_node ) {
            // See https://mathml-refresh.github.io/mathml-core/#base-with-underscript
            int gap = 0;
            if ( under_is_accent ) {
                // No specific metric, but let's ensure min 1px added to void glyph merging
                gap = 1;
            }
            else {
                gap = base_node->getFont()->getExtraMetric(font_metric_math_underbar_vertical_gap);
            }
            if ( is_core_largeop ) {
                int gap1 = base_node->getFont()->getExtraMetric(font_metric_math_lower_limit_baseline_drop_min);
                gap1 = gap1 != 0 ? gap1 - under_ink_ascent : gap; // fallback to gap if metric absent
                int gap2 = base_node->getFont()->getExtraMetric(font_metric_math_lower_limit_gap_min);
                gap = gap1 > gap2 ? gap1 : gap2;
            }
            else if ( is_core_hstretchy ) {
                int gap1 = base_node->getFont()->getExtraMetric(font_metric_math_stretch_stack_bottom_shift_down);
                gap1 = gap1 != 0 ? gap1 - under_ink_ascent : gap; // fallback to gap if metric absent
                int gap2 = base_node->getFont()->getExtraMetric(font_metric_math_stretch_stack_gap_above_min);
                gap = gap1 > gap2 ? gap1 : gap2;
            }
            base_row->height += gap;
            base_cell->height += gap;
            if ( !is_core_largeop && !is_core_hstretchy ) {
                // padding at bottom of under
                gap = base_node->getFont()->getExtraMetric(font_metric_math_underbar_extra_descender);
                under_row->height += gap;
                under_cell->height += gap;
            }
        }

        if ( mathml_tweaked_element_name_id != el_munder ) {
            // The DOM really has a single row. Set this row baseline to
            // be the one of the 2nd row we added for rendering.
            rows[0]->baseline = rows[0]->height + rows[1]->baseline;
        }
    }

    // mfrac
    else if ( mathml_tweaked_element_name_id == el_mfrac ) {
        CCRTableRow * num_row = rows[0];
        CCRTableRow * den_row = rows[1];
        CCRTableCell * num_cell = num_row->cells[0];
        CCRTableCell * den_cell = den_row->cells[0];
        ldomNode * num_node = num_cell->elem;
        ldomNode * den_node = den_cell->elem;
        LVFontRef frac_font = elem->getFont();

        // We have computed the linethicknes, it's available as the top margin of the denominator
        int linethickness = 0;
        css_style_ref_t den_style = den_node->getStyle();
        if ( den_style->border_style_top != css_border_none ) {
            linethickness = lengthToPx(elem, den_style->border_width[0], 0);
        }

        if ( linethickness > 0 ) {
            // https://mathml-refresh.github.io/mathml-core/#fraction-with-nonzero-line-thickness
            int upper_half_linethickness = linethickness / 2;

            // Get metrics from the <mfrac> font
            int axis_height = frac_font->getExtraMetric(font_metric_math_axis_height);
            // Different metrics depending of math-style: normal/compact
            int num_shift_up, num_gap_min;
            int den_shift_down, den_gap_min;
            if ( elem->hasAttribute(attr_MD) ) { // displaystyle=true / math-style: normal
                num_shift_up = frac_font->getExtraMetric(font_metric_math_fraction_numerator_display_style_shift_up);
                num_gap_min = frac_font->getExtraMetric(font_metric_math_fraction_num_display_style_gap_min);
                den_shift_down = frac_font->getExtraMetric(font_metric_math_fraction_denominator_display_style_shift_down);
                den_gap_min = frac_font->getExtraMetric(font_metric_math_fraction_denom_display_style_gap_min);
            }
            else { // math-style: compact
                num_shift_up = frac_font->getExtraMetric(font_metric_math_fraction_numerator_shift_up);
                num_gap_min = frac_font->getExtraMetric(font_metric_math_fraction_numerator_gap_min);
                den_shift_down = frac_font->getExtraMetric(font_metric_math_fraction_denominator_shift_down);
                den_gap_min = frac_font->getExtraMetric(font_metric_math_fraction_denominator_gap_min);
            }

            lvRect num_ink_offsets;
            bool num_has_ink = getInkOffsets( num_node, num_ink_offsets, true);
            if ( num_has_ink ) {
                int num_baseline_to_bottom = num_cell->height - num_cell->baseline; // using original baseline
                int num_ink_descent = num_baseline_to_bottom - num_ink_offsets.bottom;
                // The distance from the the outer baseline to the numerator baseline
                // is to be the max of:
                // - num_shift_up
                // - axis_height + half of linethickness + num_gap_min + num_ink_descent
                // Working from the cell bottom (from the top of the fraction line):
                int baseline_to_bottom_1 = num_shift_up - axis_height - upper_half_linethickness;
                int baseline_to_bottom_2 = num_gap_min + num_ink_descent;
                int baseline_to_bottom = baseline_to_bottom_1 > baseline_to_bottom_2 ? baseline_to_bottom_1 : baseline_to_bottom_2;
                // To change the distance from baseline to bottom, we just need to change
                // the cell and row height to add/reduce the pad at the bottom
                int d_height = baseline_to_bottom - num_baseline_to_bottom;
                RenderRectAccessor num_fmt( num_node );
                num_fmt.setHeight( num_fmt.getHeight() + d_height );
                num_cell->height += d_height;
                num_row->height += d_height;
            }
            lvRect den_ink_offsets;
            bool den_has_ink = getInkOffsets( den_node, den_ink_offsets, true, false, true);
                        // skip_initial_borders=true to ignore the top border (the fraction line)
            if ( den_has_ink ) {
                int den_top_to_baseline = den_cell->baseline;
                int den_ink_ascent = den_cell->baseline - den_ink_offsets.top;
                // The distance from the the outer baseline to the denominator baseline
                // is to be the max of:
                // - den_shift_down
                // - half of linethickness + den_gap_min + den_ink_ascent - axis_height
                // Working from the cell top (from the top of the fraction line, including it in full):
                int top_to_baseline_1 = upper_half_linethickness + axis_height + den_shift_down;
                int top_to_baseline_2 = linethickness + den_gap_min + den_ink_ascent;
                int top_to_baseline = top_to_baseline_1 > top_to_baseline_2 ? top_to_baseline_1 : top_to_baseline_2;
                // To change the distance from top to the inner content baseline, we update
                // the inner y (where the erm_final content will start being draw).
                // We should also update this addition/reduction in the heights
                int d_baseline_y = top_to_baseline - den_top_to_baseline;
                RenderRectAccessor den_fmt( den_node );
                den_fmt.setInnerY( den_fmt.getInnerY() + d_baseline_y );
                den_fmt.setHeight( den_fmt.getHeight() + d_baseline_y );
                den_cell->height += d_baseline_y;
                den_row->height += d_baseline_y;
            }
            // Update the first row baseline, which will set the table outer baseline
            rows[0]->baseline = rows[0]->height + upper_half_linethickness + axis_height;
        }
        else {
            // https://mathml-refresh.github.io/mathml-core/#fraction-with-zero-line-thickness
            // Different metrics depending of math-style: normal/compact
            int top_shift_up, bottom_shift_down, gap_min;
            if ( elem->hasAttribute(attr_MD) ) { // displaystyle=true / math-style: normal
                top_shift_up = frac_font->getExtraMetric(font_metric_math_stack_top_display_style_shift_up);
                bottom_shift_down = frac_font->getExtraMetric(font_metric_math_stack_bottom_display_style_shift_down);
                gap_min = frac_font->getExtraMetric(font_metric_math_stack_display_style_gap_min);
            }
            else { // math-style: compact
                top_shift_up = frac_font->getExtraMetric(font_metric_math_stack_top_shift_up);
                bottom_shift_down = frac_font->getExtraMetric(font_metric_math_stack_bottom_shift_down);
                gap_min = frac_font->getExtraMetric(font_metric_math_stack_gap_min);
            }

            int num_baseline_to_bottom = num_cell->height - num_cell->baseline; // using original baseline
            int num_ink_descent = 0;
            lvRect num_ink_offsets;
            bool num_has_ink = getInkOffsets( num_node, num_ink_offsets, true);
            if ( num_has_ink ) {
                num_ink_descent = num_baseline_to_bottom - num_ink_offsets.bottom;
            }
            int den_top_to_baseline = den_cell->baseline;
            int den_ink_ascent = 0;
            lvRect den_ink_offsets;
            bool den_has_ink = getInkOffsets( den_node, den_ink_offsets, true);
            if ( den_has_ink ) {
                den_ink_ascent = den_cell->baseline - den_ink_offsets.top;
            }
            // The rules come down to:
            // - the ideal distance between baselines is top_shift_up + bottom_shift_down
            // - the min distance from inks is gap_min; if gap is smaller, distribute on both sides
            // - the mfrac baseline is adjusted top_shift down the baseline of the first row
            int baselines_dist = top_shift_up + bottom_shift_down; // ideal
            int gap = baselines_dist - num_ink_descent - den_ink_ascent;
            int top_shift_up_add = 0;
            int d_gap = gap_min - gap;
            if ( d_gap > 0 ) {
                baselines_dist = num_ink_descent + gap_min + den_ink_ascent; // ensuring gap_min
                top_shift_up_add = d_gap / 2;
            }
            int cur_baselines_dist = num_baseline_to_bottom + den_top_to_baseline;
            // So, we should be able to ensure all that by just modifying the first cell/row height,
            // and setting the first row baseline appropriately.
            int d_height = baselines_dist - cur_baselines_dist;
            RenderRectAccessor num_fmt( num_node );
            num_fmt.setHeight( num_fmt.getHeight() + d_height );
            num_cell->height += d_height;
            num_row->height += d_height;
            rows[0]->baseline = num_cell->baseline + top_shift_up + top_shift_up_add;
        }
    }
}

// Called at end of rendering, after rows got their height and y
// relative to their table container.
void CCRTable::MathML_finalizeTableLayout() {
    // Some cells may have been put in an added row that does not map
    // to a node. Fix up RenderRectAccessors positionning and sizes.
    // We assume these cells' nodes are part of the last previous row
    // that has a node, and we need to adjust these cells x/y to be
    // relative to this real row (their real container in the DOM),
    // and adjust this real row height to also span the fake rows.
    CCRTableRow * last_real_row = NULL;
    for (int i=0; i<rows.length(); i++) {
        CCRTableRow * row = rows[i];
        bool is_real_row;
        int shift_from_last_real_row;
        if ( row->elem ) {
            is_real_row = true;
            last_real_row = row;
            shift_from_last_real_row = 0;
        }
        else {
            is_real_row = false;
            if ( !last_real_row ) // should not happen
                continue;
            shift_from_last_real_row = row->y - last_real_row->y;
        }
        RenderRectAccessor fmt( last_real_row->elem );
        RENDER_RECT_SET_FLAG(fmt, CHILDREN_RENDERING_REORDERED);
        // Update last row height
        int last_real_row_overflow_y = last_real_row->height + last_real_row->bottom_overflow;
        if ( !is_real_row ) {
            last_real_row->height += row->height;
            fmt.setHeight( last_real_row->height );
        }
        int min_cell_top_y = 0;
        int max_cell_bottom_y = 0;
        for (int j=0; j<rows[i]->cells.length(); j++) {
            CCRTableCell * cell = rows[i]->cells[j];
            if ( !cell->elem ) // might be an empty cell added by MathML tweaks
                continue;
            if ( cell->row->index != i )
                continue;
            RenderRectAccessor cfmt( cell->elem );
            int cell_y = cfmt.getY() + shift_from_last_real_row;
            int cell_h = cfmt.getHeight();
            cfmt.setY( cell_y );
            // Keep track of highest and lowest cells area, to possibly update the row's overflows
            if ( min_cell_top_y > cell_y )
                min_cell_top_y = cell_y;
            if ( max_cell_bottom_y < cell_y + cell_h )
                max_cell_bottom_y = cell_y + cell_h;
            // With the mathml positionning tweaks, a cell might be fully outside its row.
            // Update its overflows so they intersect with the row, so this cell is not
            // skipped by DrawDocument.
            if ( cell_y + cell_h <= 0 ) {
                // cell fully above row, set bottom overflow so it intersects with row
                cfmt.setBottomOverflow( 1 - (cell_y + cell_h) );
            }
            if ( cell_y >= last_real_row->height ) {
                // cell fully below row, set top overflow so it intersects with row
                cfmt.setTopOverflow( 1 + cell_y - last_real_row->height );
            }
        }
        if ( enhanced_rendering ) {
            // Update last row bottom overflow, for rowspans and any cells from other
            // virtual rows that may draw outside of this row height
            int row_max_overflow_y = shift_from_last_real_row + row->height + row->bottom_overflow;
            if ( row_max_overflow_y < max_cell_bottom_y )
                row_max_overflow_y = max_cell_bottom_y;
            if ( last_real_row_overflow_y < row_max_overflow_y )
                last_real_row->bottom_overflow = row_max_overflow_y - last_real_row->height;
            fmt.setBottomOverflow( last_real_row->bottom_overflow );
            // Update last row top overflow, for any cells from other virtual rows
            if ( min_cell_top_y < 0 && fmt.getTopOverflow() < -min_cell_top_y )
                fmt.setTopOverflow( -min_cell_top_y );
        }
    }
    // Here is a good place to ensure horizontal stretching of <mo> contained
    // in nested munder/mover/munderover, as the table is nearly fully set up
    if ( mathml_tweaked_element_name_id >= el_munder && mathml_tweaked_element_name_id <= el_munderover
                && elem->hasAttribute(attr_Mhas_hstretch) ) {
        ensureMathMLInnerMOsHorizontalStretch(elem); // we provide the table node
    }
    // And also horizontal or vertical stretching of standalone <mo> contained in <mtd>
    if ( mathml_tweaked_element_name_id == el_mtable ) {
        for (int i=0; i<rows.length(); i++) {
            for (int j=0; j<rows[i]->cells.length(); j++) {
                CCRTableCell * cell = rows[i]->cells[j];
                if ( !cell->elem )
                    continue;
                int y = cell->row->index;
                if ( i==y ) {
                    if ( cell->elem->getRendMethod() == erm_final ) {
                        ensureMathMLMOInMTDStretch( cell->elem );
                    }
                }
            }
        }
    }
}

void MathML_fixupTableLayoutForRenderedWidths( lUInt16 nodeElementId, ldomNode * node, void * tab, int seen_nb_cells ) {
    // We only need tweaks for MathML elements, and only some of them
    if ( nodeElementId < el_munder || nodeElementId > el_mmultiscripts ) {
        return;
    }

    // typedefs copied from getRenderedWidths()'s erm_table section: keep them sync'ed
    // (We just don't need to have them public. This could be inline'd in getRenderedWidths(),
    // but we want MathML specific stuff externalized.)
    typedef struct CellWidths {
        int min_w;
        int max_w;
        int colspan;
        int rowspan;
        int last_row_idx; // when used as column: index of last row occupied by previous rowspans
        ldomNode * elem;
        CellWidths() : min_w(0), max_w(0), colspan(1), rowspan(1), last_row_idx(-1), elem(NULL) {};
        CellWidths(int min, int max, int cspan=1, int rspan=1, ldomNode * n=NULL )
            : min_w(min), max_w(max), colspan(cspan), rowspan(rspan), last_row_idx(-1), elem(n) {};
    } CellWidths;
    typedef LVArray<CellWidths> RowCells;
    typedef LVArray<RowCells> getRenderedWidths_table;

    // Get the table back as it was in getRenderedWidths(), from the passed void*
    getRenderedWidths_table * ptable = (getRenderedWidths_table *)tab;
    getRenderedWidths_table & table = *ptable;

    // Some MathML elements are set to be inline-table, and we might do some
    // internal re-ordering of rows and cells in the table rendering code via
    // the above added CCRTable methods.
    // We need to do some bits of this re-ordering, just enough so the
    // widths will would be the same.

    // msub/msup/msubsup/mmultiscripts, or munder/mover/munderover that should be rendered as msub/msup/msupsup
    if ( (nodeElementId >= el_msub && nodeElementId <= el_mmultiscripts) ||
         (nodeElementId >= el_munder && nodeElementId <= el_munderover && node->hasAttribute(attr_Msubsup) ) ) {
        // As in CCRTable::MathML_checkAndTweakTableElement(), but just enough
        // to get something that will measure the same:
        // we use only one row and don't care about re-ordering after <mprescripts/>
        table[0][0].rowspan = 2;
        RowCells row2; // row for subscripts
        row2.reserve(seen_nb_cells);
        table.add(row2);
        bool next_is_sup = nodeElementId == el_msup || nodeElementId == el_mover;
        int i = 1;
        while (i < table[0].length()) {
            if (table[0][i].elem && table[0][i].elem->getNodeId() == el_mprescripts) {
                if (next_is_sup)
                    i++; // keep it in row1
                else
                    table[0].remove(i); // remove it
                next_is_sup = false;
                continue;
            }
            if (next_is_sup)
                i++; // let this cell be where it was in row1
            else {
                // Move it to subscripts row
                CellWidths cell = table[0].remove(i);
                table[1].add( cell );
            }
            next_is_sup = !next_is_sup;
        }
    }

    // Normal munder/mover/munderover
    else if ( nodeElementId >= el_munder && nodeElementId <= el_munderover ) {
        // As in CCRTable::MathML_checkAndTweakTableElement(), but just enough
        // to get something that will measure the same:
        // we put each cell of the single row into a row with a single cell
        for ( int i=table[0].length()-1; i > 0; i-- ) {
            RowCells row;
            table.add(row);
            CellWidths cell = table[0].remove(i);
            table[table.length()-1].add( cell );
        }
    }
}

void MathML_updateBaseline( ldomNode * node, int & baseline ) {
    // Skip the inlineBox container of our inline-table/block elements
    if ( node && node->isBoxingNode() ) {
        node = node->getUnboxedFirstChild();
    }
    if ( !node )
        return;
    lUInt16 nodeElementId = node->getNodeId();
    if ( nodeElementId == el_mspace ) {
        // As in setMathMLElementNodeStyle()
        css_length_t h;
        lString32 at_height = node->getAttributeValueLC(attr_height);
        bool has_height = getLengthFromMathMLAttributeValue(at_height, h, false) // no % (default value = 0)
                                    && h.type != css_val_unspecified && h.value != 0;
        if ( has_height ) {
            // The baseline is actually just the MathML height
            baseline = lengthToPx(node, h, 0);
            return;
        }
        css_length_t d;
        lString32 at_depth = node->getAttributeValueLC(attr_depth);
        bool has_depth = getLengthFromMathMLAttributeValue(at_depth, d, false) // no % (default value = 0)
                                    && d.type != css_val_unspecified && d.value != 0;
        if ( has_depth ) {
            // No height but depth: all CSS height is below baseline
            baseline = 0;
            return;
        }
        // No height, no depth: let it be as it was computed
    }
}
