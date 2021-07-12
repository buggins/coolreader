/***************************************************************************
   CoolReader Engine

   crlocaledata.cpp:  parsing and comparision locales

   (c) Aleksey Chernov, 2021
   This source code is distributed under the terms of
   GNU General Public License
   See LICENSE file for details

 ***************************************************************************/

#include "crlocaledata.h"

#if USE_LOCALE_DATA==1

#include "lvstring8collection.h"

#include "iso-639-2_data.c"
#include "iso-639-3_data.c"
#include "iso-3166-1_data.c"
#include "iso-15924_data.c"

CRLocaleData::CRLocaleData(const char* langtag)
{
    parseTag(lString8(langtag));
}

CRLocaleData::CRLocaleData(const lString8 &langtag)
{
    parseTag(langtag);
}

lString8 CRLocaleData::langTag() const
{
    if (m_lang_code.empty())
        return lString8::empty_str;

    lString8 tag;

    // part 1: main language
    if (!m_lang_part1.empty())
        tag = m_lang_part1;
    else
        tag = m_lang_code;

    // part2: script
    if (!m_script_code.empty()) {
        tag.append("-");
        tag.append(m_script_code);
    }

    // part 3: region/country
    if (!m_region_alpha2.empty()) {
        tag.append("-");
        tag.append(m_region_alpha2);
    } else if (!m_region_alpha3.empty()) {
        tag.append("-");
        tag.append(m_region_alpha3);
    }

    return tag;
}

int CRLocaleData::calcMatch(const CRLocaleData& other) const
{
    lString8 this_lang_code_lc = m_lang_code;
    this_lang_code_lc = this_lang_code_lc.lowercase();
    lString8 other_lang_code_lc = other.langCode();
    other_lang_code_lc = other_lang_code_lc.lowercase();
    if (this_lang_code_lc.compare(other_lang_code_lc) != 0)
        return 0;
    int match = 100;
    if (m_script_num != 0 && other.scriptNumeric() != 0) {
        if (m_script_num == other.scriptNumeric())
            match += 10;
        else
            match -= 10;
    }
    if (m_region_num != 0 && other.regionNumeric() != 0) {
        if (m_region_num == other.regionNumeric())
            match += 1;
        else
            match -= 1;
    }
    return match;
}

void CRLocaleData::parseTag(const lString8& langtag)
{
    m_langtag_src = langtag;
    lString8 tag = langtag;
    bool lang_ok = false;
    bool script_ok = true;
    bool region_ok = true;

    // 0. clear all fields
    m_isValid = false;
    m_lang_code = lString8::empty_str;
    m_lang_part2b = lString8::empty_str;
    m_lang_part2t = lString8::empty_str;
    m_lang_part1 = lString8::empty_str;
    m_lang_name = lString8::empty_str;

    m_script_code = lString8::empty_str;
    m_script_name = lString8::empty_str;
    m_script_alias = lString8::empty_str;
    m_script_num = 0;

    m_region_name = lString8::empty_str;
    m_region_alpha2 = lString8::empty_str;
    m_region_alpha3 = lString8::empty_str;
    m_region_num = 0;

    // 1. replace '_' with '-'
    tag = tag.replace('_', '-');

    // 2. split to components
    lString8Collection list(tag, cs8("-"));

    // 3. Analyze each part

    /* pass state codes:
     * 0 - start
     * 1 - language parsed
     * 2 - script parsed
     * 3 - region parsed
     */
    int state = 0;
    int i;
    bool part_parsed;
    int it;
    for (it = 0; it < list.length(); ++it) {
        lString8 part = list[it];
        if (part.empty())
            continue;
        part_parsed = false;
        while (!part_parsed) {
            switch (state) {
                case 0: {
                    // search language code in ISO-639-3
                    part = part.lowercase();
                    const struct iso639_3_rec* ptr = &iso639_3_data[0];
                    const struct iso639_3_rec* rec3 = NULL;
                    const struct iso639_2_rec* rec2 = NULL;
                    for (i = 0; i < ISO639_3_DATA_SZ; i++) {
                        if (part.compare(ptr->id) == 0) {
                            rec3 = ptr;
                            break;
                        } else if (part.compare(ptr->part1) == 0) {
                            rec3 = ptr;
                            break;
                        } else if (part.compare(ptr->part2b) == 0) {
                            rec3 = ptr;
                            break;
                        }
                        // don't test Part/2T, ISO-639-3 id already use Part/2T
                        ptr++;
                    }
                    if (!rec3) {
                        // if not found by code => search by full name
                        ptr = &iso639_3_data[0];
                        for (i = 0; i < ISO639_3_DATA_SZ; i++) {
                            lString8 ref_name(ptr->ref_name);
                            ref_name = ref_name.lowercase();
                            if (part.compare(ref_name) == 0) {
                                rec3 = ptr;
                                break;
                            }
                            ptr++;
                        }
                    }
                    if (!rec3) {
                        // if not found in ISO-639-3 => search in ISO-639-2 (for scope "collective")
                        // search only by id, part1
                        const struct iso639_2_rec* ptr2 = &iso639_2_data[0];
                        for (i = 0; i < ISO639_2_DATA_SZ; i++) {
                            if (part.compare(ptr2->id) == 0) {
                                rec2 = ptr2;
                                break;
                            } else if (part.compare(ptr2->part1) == 0) {
                                rec2 = ptr2;
                                break;
                            }
                            ptr2++;
                        }
                    }
                    if (rec3 || rec2)
                        lang_ok = true;
                    else if (ISO639_3_UND_INDEX >= 0)
                        rec3 = &iso639_3_data[ISO639_3_UND_INDEX];
                    if (rec3) {
                        m_lang_code = lString8(rec3->id);
                        m_lang_part2b = rec3->part2b != NULL ? lString8(rec3->part2b) : lString8::empty_str;
                        m_lang_part2t = rec3->part2t != NULL ? lString8(rec3->part2t) : lString8::empty_str;
                        m_lang_part1 = rec3->part1 != NULL ? lString8(rec3->part1) : lString8::empty_str;
                        m_lang_name = lString8(rec3->ref_name);
                    } else if (rec2) {
                        m_lang_code = lString8(rec2->id);
                        m_lang_part2b = lString8::empty_str;
                        m_lang_part2t = rec2->part2t != NULL ? lString8(rec2->part2t) : lString8::empty_str;
                        m_lang_part1 = rec2->part1 != NULL ? lString8(rec2->part1) : lString8::empty_str;
                        m_lang_name = lString8(rec2->ref_name);
                    }
                    part_parsed = true;
                    state = 1;
                    break;
                }
                case 1: {
                    // search script in ISO-15924
                    part = part.lowercase();
                    if (part.length() == 4) {
                        script_ok = false;
                        const struct iso15924_rec* ptr = &iso15924_data[0];
                        const struct iso15924_rec* rec = NULL;
                        for (i = 0; i < ISO15924_DATA_SZ; i++) {
                            lString8 code(ptr->code);
                            code = code.lowercase();
                            if (part.compare(code) == 0) {
                                rec = ptr;
                                break;
                            }
                            ptr++;
                        }
                        if (rec) {
                            // script is found
                            m_script_code = lString8(rec->code);
                            m_script_name = rec->name != NULL ? lString8(rec->name) : lString8::empty_str;
                            m_script_alias = rec->alias != NULL ? lString8(rec->alias) : lString8::empty_str;
                            m_script_num = rec->num;
                            part_parsed = true;
                            script_ok = true;
                        }
                    }
                    state = 2;
                    break;
                }
                case 2:
                    // search region/country code in ISO-3166-1
                    part = part.uppercase();
                    region_ok = false;
                    const struct iso3166_1_rec* ptr = &iso3166_1_data[0];
                    const struct iso3166_1_rec* rec = NULL;
                    for (i = 0; i < ISO3166_1_DATA_SZ; i++) {
                        lString8 name(ptr->name);
                        name = name.uppercase();
                        if (part.compare(ptr->alpha2) == 0) {
                            rec = ptr;
                            break;
                        } else if (part.compare(ptr->alpha3) == 0) {
                            rec = ptr;
                            break;
                        } else if (part.compare(name) == 0) {
                            rec = ptr;
                            break;
                        }
                        ptr++;
                    }
                    if (rec) {
                        // region is found
                        m_region_name = lString8(rec->name);
                        m_region_alpha2 = lString8(rec->alpha2);
                        m_region_alpha3 = lString8(rec->alpha3);
                        m_region_num = rec->num;
                        region_ok = true;
                    }
                    part_parsed = true;
                    state = 1;
                    break;
            }
        }
        if (3 == state) {
            // ignore all trailing tags
            break;
        }
    }
    m_isValid = lang_ok && script_ok && region_ok;
}

#endif  // #if USE_LOCALE_DATA==1
