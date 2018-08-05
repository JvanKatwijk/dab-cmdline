
#include "dab_tables.h"

#include <string.h>
#include <stdlib.h>


// ETSI TS 101 756 V2.2.1: Registered Tables
// for Extended Country Code, Country Id, Language Code, Program Type, User Application Type, Content Type
// https://www.etsi.org/deliver/etsi_ts/101700_101799/101756/02.02.01_60/ts_101756v020201p.pdf

// ETSI TR 101 496-3 V1.1.2
// protection level/classes, ..
// https://www.etsi.org/deliver/etsi_tr/101400_101499/10149603/01.01.02_60/tr_10149603v010102p.pdf

static const char *uep_rates [] = {"7/20", "2/5", "1/2", "3/5"};
static const char *eep_Arates[] = {"1/4",  "3/8", "1/2", "3/4"};
static const char *eep_Brates[] = {"4/9",  "4/7", "4/6", "4/5"};

// #include	<stdint.h>

struct country_codes {
    uint8_t ecc;
    uint8_t countryId;
    const char *countryName;
};

static country_codes countryTable[] = {
// ITU Region 1 - European broadcasting area
{0xE0, 0x9, "Albania"},
{0xE0, 0x2, "Algeria"},
{0xE0, 0x3, "Andorra"},
{0xE0, 0xA, "Austria"},
{0xE4, 0x8, "Azores"},
{0xE0, 0x6, "Belgium"},
{0xE3, 0xF, "Belarus"},
{0xE4, 0xF, "Bosnia"},
{0xE0, 0x9, "Albania"},
{0xE1, 0x8, "Bulgaria"},
{0xE2, 0xE, "Canaries"},
{0xE3, 0xC, "Croatia"},
{0xE1, 0x2, "Cyprus"},
{0xE2, 0x2, "Czech Republic"},
{0xE1, 0x9, "Denmark"},
{0xE0, 0xF, "Egypt"},
{0xE4, 0x2, "Estonia"},
{0xE1, 0x9, "Faroe"},
{0xE1, 0x6, "Finland"},
{0xE1, 0xF, "France"},
{0xE0, 0xD, "Germany"},
{0xE0, 0x1, "Germany"},
{0xE1, 0xA, "Gibraltar"},
{0xE1, 0x1, "Greece"},
{0xE0, 0xB, "Hungary"},
{0xE2, 0xA, "Iceland"},
{0xE1, 0xB, "Iraq"},
{0xE3, 0x2, "Ireland"},
{0xE0, 0x4, "Israel"},
{0xE0, 0x5, "Italy"},
{0xE1, 0x5, "Jordan"},
{0xE3, 0x9, "Latvia"},
{0xE3, 0xA, "Lebanon"},
{0xE1, 0xD, "Libya"},
{0xE2, 0x9, "Liechtenstein"},
{0xE2, 0xC, "Lithuania"},
{0xE1, 0x7, "Luxembourg"},
{0xE3, 0x4, "Macedonia"},
{0xE4, 0x8, "Madeira"},
{0xE0, 0xC, "Malta"},
{0xE2, 0x1, "Morocco"},
{0xE4, 0x1, "Moldova"},
{0xE2, 0xB, "Monaco"},
{0xE3, 0x1, "Montenegro"},
{0xE3, 0x8, "Netherlands"},
{0xE2, 0xF, "Norway"},
{0xE2, 0x3, "Poland"},
{0xE4, 0x8, "Portugal"},
{0xE1, 0xE, "Romania"},
{0xE0, 0x7, "Russian Federation"},
{0xE1, 0x3, "San Marino"},
{0xE2, 0xD, "Serbia"},
{0xE4, 0x9, "Slovenia"},
{0xE2, 0x5, "Slovak Republic"},
{0xE2, 0xE, "Spain"},
{0xE3, 0xE, "Sweden"},
{0xE1, 0x4, "Switzerland"},
{0xE2, 0x6, "Syria"},
{0xE2, 0x7, "Tunisia"},
{0xE3, 0x3, "Turkey"},
{0xE4, 0x6, "Ukraine"},
{0xE1, 0xC, "United Kingdom"},
{0xE2, 0x4, "Vatican"},

// @TODO: add following tables
// ITU Region 1 - African broadcasting area
// ITU Region 2 (North and South Americas)
// ITU Region 3 (Asia and Pacific)

{0x00, 0x0, nullptr }
};



const char * getASCTy(int16_t ASCTy)
{
    switch (ASCTy)
    {
    case 0:     return "DAB";
    case 63:    return "DAB+";
    default:    return "unknown";
    }
}

const char * getDSCTy(int16_t DSCTy)
{
    switch (DSCTy)
    {
    case 5:     return "Transparent Data Channel (TDC)";
    case 24:    return "MPEG-2 Transport Stream";
    case 60:    return "Multimedia Object Transfer (MOT)";
    case 61:    return "Proprietary service";
    default:    return "unknown";
    }
}

const char * getLanguage(int16_t language)
{
    switch (language)
    {
    case 0x00:	return "Unknown/not applicable";
    case 0x01:	return "Albanian";
    case 0x02:	return "Breton";
    case 0x03:	return "Catalan";
    case 0x04:	return "Croatian";
    case 0x05:	return "Welsh";
    case 0x06:	return "Czech";
    case 0x07:	return "Danish";
    case 0x08:	return "German";
    case 0x09:	return "English";
    case 0x0A:	return "Spanish";
    case 0x0B:	return "Esperanto";
    case 0x0C:	return "Estonian";
    case 0x0D:	return "Basque";
    case 0x0E:	return "Faroese";
    case 0x0F:	return "French";
    case 0x10:	return "Frisian";
    case 0x11:	return "Irish";
    case 0x12:	return "Gaelic";
    case 0x13:	return "Galician";
    case 0x14:	return "Icelandic";
    case 0x15:	return "Italian";
    case 0x16:	return "Sami";
    case 0x17:	return "Latin";
    case 0x18:	return "Latvian";
    case 0x19:  return "Luxembourgian";
    case 0x1A:  return "Lithuanian";
    case 0x1B:  return "Hungarian";
    case 0x1C:  return "Maltese";
    case 0x1D:  return "Dutch";
    case 0x1E:  return "Norwegian";
    case 0x1F:  return "Occitan";
    case 0x20:  return "Polish";
    case 0x21:  return "Portuguese";
    case 0x22:  return "Romanian";
    case 0x23:  return "Romansh";
    case 0x24:  return "Serbian";
    case 0x25:  return "Slovak";
    case 0x26:  return "Slovene";
    case 0x27:  return "Finnish";
    case 0x28:  return "Swedish";
    case 0x29:  return "Turkish";
    case 0x2A:  return "Flemish";
    case 0x2B:  return "Walloon";
    case 0x30:  // no break
    case 0x31:  // no break
    case 0x32:  // no break
    case 0x33:  // no break
    case 0x34:  // no break
    case 0x35:  // no break
    case 0x36:  // no break
    case 0x37:  // no break
    case 0x38:  // no break
    case 0x39:  // no break
    case 0x3A:  // no break
    case 0x3B:  // no break
    case 0x3C:  // no break
    case 0x3D:  // no break
    case 0x3E:  // no break
    case 0x3F:  return "Reserved for national assignment";
    case 0x7F:  return "Amharic";
    case 0x7E:  return "Arabic";
    case 0x7D:  return "Armenian";
    case 0x7C:  return "Assamese";
    case 0x7B:  return "Azerbaijani";
    case 0x7A:  return "Bambora";
    case 0x79:  return "Belorussian";
    case 0x78:  return "Bengali";
    case 0x77:  return "Bulgarian";
    case 0x76:  return "Burmese";
    case 0x75:  return "Chinese";
    case 0x74:  return "Chuvash";
    case 0x73:  return "Dari";
    case 0x72:  return "Fulani";
    case 0x71:  return "Georgian";
    case 0x70:  return "Greek";
    case 0x6F:  return "Gujurati";
    case 0x6E:  return "Gurani";
    case 0x6D:  return "Hausa";
    case 0x6C:  return "Hebrew";
    case 0x6B:  return "Hindi";
    case 0x6A:  return "Indonesian";
    case 0x69:  return "Japanese";
    case 0x68:  return "Kannada";
    case 0x67:  return "Kazakh";
    case 0x66:  return "Khmer";
    case 0x65:  return "Korean";
    case 0x64:  return "Laotian";
    case 0x63:  return "Macedonian";
    case 0x62:  return "Malagasay";
    case 0x61:  return "Malaysian";
    case 0x60:  return "Moldavian";
    case 0x5F:  return "Marathi";
    case 0x5E:  return "Ndebele";
    case 0x5D:  return "Nepali";
    case 0x5C:  return "Oriya";
    case 0x5B:  return "Papiamento";
    case 0x5A:  return "Persian";
    case 0x59:  return "Punjabi";
    case 0x58:  return "Pushtu";
    case 0x57:  return "Quechua";
    case 0x56:  return "Russian";
    case 0x55:  return "Rusyn";
    case 0x54:  return "Serbo-Croat";
    case 0x53:  return "Shona";
    case 0x52:  return "Sinhalese";
    case 0x51:  return "Somali";
    case 0x50:  return "Sranan Tongo";
    case 0x4F:  return "Swahili";
    case 0x4E:  return "Tadzhik";
    case 0x4D:  return "Tamil";
    case 0x4C:  return "Tatar";
    case 0x4B:  return "Telugu";
    case 0x4A:  return "Thai";
    case 0x49:  return "Ukranian";
    case 0x48:  return "Urdu";
    case 0x47:  return "Uzbek";
    case 0x46:  return "Vietnamese";
    case 0x45:  return "Zulu";
    case 0x40:  return "Background sound/clean feed";
    default:    return "unknown";
    }
}

const char * getCountry(uint8_t ecc, uint8_t countryId)
{
    int16_t	i = 0;
    while (countryTable[i].ecc != 0)
    {
       if ( (countryTable[i].ecc == ecc) && (countryTable[i].countryId == countryId) )
           return countryTable[i].countryName;
       ++i;
    }
    return nullptr;
}


const char * getProgramType_Not_NorthAmerica(int16_t programType)
{
    switch (programType)
    {
    case 0:     return "No programme type";
    case 1:     return "News";
    case 2:     return "Current Affairs";
    case 3:     return "Information";
    case 4:     return "Sport";
    case 5:     return "Education";
    case 6:     return "Drama";
    case 7:     return "Culture";
    case 8:     return "Science";
    case 9:     return "Varied";    //Talk
    case 10:    return "Pop Music";
    case 11:    return "Rock Music";
    case 12:    return "Easy Listening Music";
    case 13:    return "Light Classical";
    case 14:    return "Serious Classical";
    case 15:    return "Other Music";
    case 16:    return "Weather/meteorology";
    case 17:    return "Finance/Business";
    case 18:    return "Children's programmes";
    case 19:    return "Social Affairs";    //Factual
    case 20:    return "Religion";
    case 21:    return "Phone In";
    case 22:    return "Travel";
    case 23:    return "Leisure";
    case 24:    return "Jazz Music";
    case 25:    return "Country Music";
    case 26:    return "National Music";
    case 27:    return "Oldies Music";
    case 28:    return "Folk Music";
    case 29:    return "Documentary";
    default:    return "unknown";
    }
}

const char * getProgramType_For_NorthAmerica(int16_t programType)
{
    switch (programType)
    {
    case 0:     return "No program type";
    case 1:     return "News";
    case 2:     return "Information";
    case 3:     return "Sports";
    case 4:     return "Talk";
    case 5:     return "Rock";
    case 6:     return "Classic Rock";
    case 7:     return "Adult Hits";
    case 8:     return "Soft Rock";
    case 9:     return "Top 40";
    case 10:    return "Country";
    case 11:    return "Oldies";
    case 12:    return "Soft";
    case 13:    return "Nostalgia";
    case 14:    return "Jazz";
    case 15:    return "Classical";
    case 16:    return "Rhythm and Blues";
    case 17:    return "Soft Rhythm and Blues";
    case 18:    return "Foreign Language";
    case 19:    return "Religious Music";
    case 20:    return "Religious Talk";
    case 21:    return "Personality";
    case 22:    return "Public";
    case 23:    return "College";
    default:    return "unknown";
    }
}

// 11-bit from HandleFIG0Extension13
const char * getUserApplicationType(int16_t appType)
{
    switch (appType)
    {
    case 1:     return "Dynamic labels (X-PAD only)";
    case 2:     return "MOT Slide Show";
    case 3:     return "MOT Broadcast Web Site";
    case 4:     return "TPEG";
    case 5:     return "DGPS";
    default:    return "unknown";
    }
}

const char * getFECscheme(int16_t FEC_scheme)
{
    switch (FEC_scheme)
    {
    case 0:     return "no FEC";
    case 1:     return "FEC";
    default:    return "unknown";
    }
}


const char * getProtectionLevel(bool shortForm, int16_t protLevel)
{
    int h = protLevel;
    if (!shortForm)
    {
        switch (h)
        {
        case 0:     return "EEP 1-A";
        case 1:     return "EEP 2-A";
        case 2:     return "EEP 3-A";
        case 3:     return "EEP 4-A";
        case 4:     return "EEP 1-B";
        case 5:     return "EEP 2-B";
        case 6:     return "EEP 3-B";
        case 7:     return "EEP 4-B";
        default:    return "EEP unknown";
        }
    }
    else
    {
        switch (h)
        {
        case 1:     return "UEP 1";
        case 2:     return "UEP 2";
        case 3:     return "UEP 3";
        case 4:     return "UEP 4";
        case 5:     return "UEP 5";
        default:    return "UEP unknown";
        }
    }
}


const char * getCodeRate(bool shortForm, int16_t protLevel)
{
    int h = protLevel;
    if (!shortForm)
        return ((h & (1 << 2)) == 0) ? eep_Arates[ h & 03 ] : eep_Brates[ h & 03 ]; // EEP -A/-B
    else
        return uep_rates[ h & 03 ];     // UEP
}

