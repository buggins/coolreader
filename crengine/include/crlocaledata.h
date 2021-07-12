/***************************************************************************
   CoolReader Engine

   crlocaledata.h:  parsing and comparision locales

   (c) Aleksey Chernov, 2021
   This source code is distributed under the terms of
   GNU General Public License
   See LICENSE file for details

 ***************************************************************************/

#ifndef CRLOCALEDATA_H
#define CRLOCALEDATA_H

#include "crsetup.h"

#if USE_LOCALE_DATA==1

#include "lvstring.h"

class CRLocaleData
{
public:
    CRLocaleData(const char* langtag);
    CRLocaleData(const lString8& langtag);
    bool isValid() const {
        return m_isValid;
    }
    lString8 langTag() const;
    const lString8& langCode() const {
        return m_lang_code;
    }
    const lString8& langPart2B() const {
        return m_lang_part2b;
    }
    const lString8& langPart2T() const {
        return m_lang_part2t;
    }
    const lString8& langPart1() const {
        return m_lang_part1;
    }
    const lString8& langName() const {
        return m_lang_name;
    }
    const lString8& scriptCode() const {
        return m_script_code;
    }
    const lString8& scriptName() const {
        return m_script_name;
    }
    const lString8& scriptAlias() const {
        return m_script_name;
    }
    unsigned int scriptNumeric() const {
        return m_script_num;
    }
    const lString8& regionAlpha2() const {
        return m_region_alpha2;
    }
    const lString8& regionAlpha3() const {
        return m_region_alpha3;
    }
    const lString8& regionName() const {
        return m_region_name;
    }
    unsigned int regionNumeric() const {
        return m_region_num;
    }
    int calcMatch(const CRLocaleData& other) const;
protected:
    void parseTag(const lString8& langtag);
private:
    bool m_isValid;

    lString8 m_langtag_src;			// full langtag (source)
    lString8 m_lang_code;			// ISO 639-3 lang id/code
    lString8 m_lang_part2b;			// Equivalent 639-2 identifier of the bibliographic application code set, if there is one
    lString8 m_lang_part2t;			// Equivalent 639-2 identifier of the terminology application code set, if there is one
    lString8 m_lang_part1;			// Equivalent 639-1 identifier, if there is one
    lString8 m_lang_name;			// Reference language name

    lString8 m_script_code;			// script code
    lString8 m_script_name;			// script name (english)
    lString8 m_script_alias;			// script alias
    unsigned int m_script_num;		// script: numeric

    lString8 m_region_name;			// region name (english)
    lString8 m_region_alpha2;		// region: alpha2 code
    lString8 m_region_alpha3;		// region: alpha2 code
    unsigned int m_region_num;		// script: numeric
};

#endif  // USE_LOCALE_DATA==1

#endif  // CRLOCALEDATA_H
