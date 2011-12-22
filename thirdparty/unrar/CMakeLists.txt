SET (UNRAR_SOURCES 
    rar.cpp strlist.cpp strfn.cpp pathfn.cpp savepos.cpp smallfn.cpp global.cpp file.cpp filefn.cpp filcreat.cpp
    archive.cpp arcread.cpp unicode.cpp system.cpp isnt.cpp crypt.cpp crc.cpp rawread.cpp encname.cpp
    resource.cpp match.cpp timefn.cpp rdwrfn.cpp consio.cpp options.cpp ulinks.cpp errhnd.cpp rarvm.cpp
    rijndael.cpp getbits.cpp sha1.cpp extinfo.cpp extract.cpp volume.cpp list.cpp find.cpp unpack.cpp cmddata.cpp
    dll.cpp
)
ADD_DEFINITIONS(-D RARDLL)
ADD_LIBRARY(unrar STATIC ${UNRAR_SOURCES})
