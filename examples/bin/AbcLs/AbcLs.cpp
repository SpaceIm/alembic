//-*****************************************************************************
//
// Copyright (c) 2013,
//  Sony Pictures Imageworks, Inc. and
//  Industrial Light & Magic, a division of Lucasfilm Entertainment Company Ltd.
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// *       Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// *       Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
// *       Neither the name of Sony Pictures Imageworks, nor
// Industrial Light & Magic nor the names of their contributors may be used
// to endorse or promote products derived from this software without specific
// prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//-*****************************************************************************

#include <Alembic/Abc/All.h>
#include <Alembic/AbcCoreAbstract/All.h>
#include <Alembic/AbcCoreHDF5/All.h>
#include <Alembic/AbcCoreOgawa/All.h>
#include <Alembic/AbcCoreFactory/All.h>
#include <Alembic/AbcCollection/All.h>
#include <Alembic/AbcGeom/All.h>
#include <Alembic/AbcMaterial/All.h>

#include <algorithm>
#include <iostream>
#include <sstream>
#include <sys/stat.h>

namespace Abc  = ::Alembic::Abc;
namespace AbcA = ::Alembic::AbcCoreAbstract;
namespace AbcF = ::Alembic::AbcCoreFactory;
namespace AbcG = ::Alembic::AbcGeom;
namespace AbcU = ::Alembic::Util;

using AbcA::index_t;

#define RESETCOLOR "\033[0m"
#define GRAYCOLOR "\033[1;30m"
#define GREENCOLOR "\033[1;32m"
#define BLUECOLOR "\033[1;34m"
#define CYANCOLOR "\033[1;36m"
#define BOLD "\033[1m"
#define COL_1 20
#define COL_2 15

//-*****************************************************************************
#define CASE_PRINT_VALUE( TPTraits, VALUE_TYPE, PROP, PARENT, HEADER, SELECTOR ) \
case TPTraits::pod_enum:                                         \
    return printSampleValue<VALUE_TYPE, PROP>( PARENT, HEADER, SELECTOR );

//-*****************************************************************************
bool is_digit(const std::string& s)
{
    std::string::const_iterator it = s.begin();
    while (it != s.end() && std::isdigit(*it)) ++it;
    return !s.empty() && it == s.end();
}

//-*****************************************************************************
void printParent( Abc::ICompoundProperty iProp,
                  bool all = false,
                  bool long_list = false,
                  bool recursive = false,
                  bool first = false )
{
    std::cout << CYANCOLOR
              << iProp.getObject().getFullName() << "/"
              << iProp.getName() << ":"
              << RESETCOLOR
              << std::endl;
}

//-*****************************************************************************
void printParent( AbcG::IObject iObj,
                  bool all = false,
                  bool long_list = false,
                  bool recursive = false,
                  bool first = false )
{
    if ( !first && !long_list )
        std::cout << std::endl;
    std::cout << CYANCOLOR
              << iObj.getFullName() << ":"
              << RESETCOLOR
              << std::endl;
}

//-*****************************************************************************
void printMetaData( AbcA::MetaData md, bool all = false,
                    bool long_list = false )
{
    std::stringstream ss( md.serialize() );
    std::string segment;
    std::string spacing( COL_1, ' ' );

    if ( long_list ) {
        if ( all )
            spacing = std::string( COL_1 + COL_2, ' ' );

        if ( md.size() == 1 ) {
            std::cout << GRAYCOLOR << " {"
                      << md.serialize()
                      << "}"
                      << RESETCOLOR;
        } else if ( md.size() > 1 ) {
            std::cout << GRAYCOLOR << " {" << std::endl;
            while ( std::getline( ss, segment, ';' ) ) {
                std::cout << spacing << " " << segment << std::endl;
            }
            std::cout << spacing << "}" << RESETCOLOR;
        }
    } else {
        std::cout << GRAYCOLOR << " {"
                  << md.serialize()
                  << "} "
                  << RESETCOLOR;
    }
}

//-*****************************************************************************
template<class PROPERTY>
void getMetaData( Abc::ICompoundProperty iParent, Abc::PropertyHeader header,
                  bool all = false, bool long_list = false )
{
    PROPERTY iProp( iParent, header.getName() );
    printMetaData( iProp.getMetaData(), all, long_list );
}

//-*****************************************************************************
template<class TYPE, class PROPERTY>
void printSampleValue( Abc::ICompoundProperty iParent, Abc::PropertyHeader header,
                       const Abc::ISampleSelector &iSS )
{
    Abc::DataType dType = header.getDataType();

    if ( header.isArray() ) {
        Abc::IArrayProperty iProp( iParent, header.getName() );
        Abc::ArraySamplePtr ptr;
        iProp.get( ptr, iSS );
        size_t numPoints = ptr->getDimensions().numPoints();
        TYPE *vals = (TYPE*)(ptr->getData());
        for ( size_t i=0; i<numPoints; ++i ) {
            std::cout << vals[i];
            if ( i > 0 && i % dType.getExtent() == 0 )
                std::cout << std::endl;
            else
                std::cout << ", ";
        }
        std::cout << std::endl << std::endl;

    } else {
        PROPERTY iProp( iParent, header.getName() );
        std::cout << iProp.getValue( iSS ) << std::endl;
    }
}

//-*****************************************************************************
void printValue( Abc::ICompoundProperty iParent, Abc::PropertyHeader header,
                 int index = 0 )
{
    Abc::ISampleSelector iss( (index_t) index );
    Abc::DataType dt = header.getDataType();
    AbcU::PlainOldDataType pod = dt.getPod();
    AbcU ::uint8_t extent = dt.getExtent();

    if ( header.isArray() ) {
        switch ( pod )
        {
            CASE_PRINT_VALUE(Abc::BooleanTPTraits, bool, Abc::IBoolArrayProperty, iParent, header, iss);
            CASE_PRINT_VALUE(Abc::Uint8TPTraits, uint8_t, Abc::IUcharArrayProperty, iParent, header, iss);
            CASE_PRINT_VALUE(Abc::Int8TPTraits, int8_t, Abc::ICharArrayProperty, iParent, header, iss);
            CASE_PRINT_VALUE(Abc::Uint16TPTraits, uint16_t, Abc::IUInt16ArrayProperty, iParent, header, iss);
            CASE_PRINT_VALUE(Abc::Int16TPTraits, int16_t, Abc::IInt16ArrayProperty, iParent, header, iss);
            CASE_PRINT_VALUE(Abc::Uint32TPTraits, uint32_t, Abc::IUInt32ArrayProperty, iParent, header, iss);
            CASE_PRINT_VALUE(Abc::Int32TPTraits, int32_t, Abc::IInt32ArrayProperty, iParent, header, iss);
            CASE_PRINT_VALUE(Abc::Uint64TPTraits, uint64_t, Abc::IUInt64ArrayProperty, iParent, header, iss);
            CASE_PRINT_VALUE(Abc::Int64TPTraits, int64_t, Abc::IInt64ArrayProperty, iParent, header, iss);
            CASE_PRINT_VALUE(Abc::Float32TPTraits, float, Abc::IFloatArrayProperty, iParent, header, iss);
            CASE_PRINT_VALUE(Abc::Float64TPTraits, double, Abc::IDoubleArrayProperty, iParent, header, iss);
            CASE_PRINT_VALUE(Abc::StringTPTraits, std::string, Abc::IStringArrayProperty, iParent, header, iss);
            default:
                std::cout << "Unknown property type" << std::endl;
                break;
        }
    } else {
        if ( extent == 1 ) {
            switch ( pod )
            {
                CASE_PRINT_VALUE(Abc::BooleanTPTraits, bool, Abc::IBoolProperty, iParent, header, iss);
                CASE_PRINT_VALUE(Abc::Uint8TPTraits, uint8_t, Abc::IUcharProperty, iParent, header, iss);
                CASE_PRINT_VALUE(Abc::Int8TPTraits, int8_t, Abc::ICharProperty, iParent, header, iss);
                CASE_PRINT_VALUE(Abc::Uint16TPTraits, uint16_t, Abc::IUInt16Property, iParent, header, iss);
                CASE_PRINT_VALUE(Abc::Int16TPTraits, int16_t, Abc::IInt16Property, iParent, header, iss);
                CASE_PRINT_VALUE(Abc::Uint32TPTraits, uint32_t, Abc::IUInt32Property, iParent, header, iss);
                CASE_PRINT_VALUE(Abc::Int32TPTraits, int32_t, Abc::IInt32Property, iParent, header, iss);
                CASE_PRINT_VALUE(Abc::Uint64TPTraits, uint64_t, Abc::IUInt64Property, iParent, header, iss);
                CASE_PRINT_VALUE(Abc::Int64TPTraits, int64_t, Abc::IInt64Property, iParent, header, iss);
                CASE_PRINT_VALUE(Abc::Float32TPTraits, float, Abc::IFloatProperty, iParent, header, iss);
                CASE_PRINT_VALUE(Abc::Float64TPTraits, double, Abc::IDoubleProperty, iParent, header, iss);
                CASE_PRINT_VALUE(Abc::StringTPTraits, std::string, Abc::IStringProperty, iParent, header, iss);
                default:
                    std::cout << "Unknown property type" << std::endl;
                    break;
            }

        } else if ( extent == 2 ) {
            switch ( pod )
            {
                CASE_PRINT_VALUE(Abc::V2sTPTraits, Imath::V2s, Abc::IV2sProperty, iParent, header, iss);
                CASE_PRINT_VALUE(Abc::V2iTPTraits, Imath::V2i, Abc::IV2iProperty, iParent, header, iss);
                CASE_PRINT_VALUE(Abc::V2fTPTraits, Imath::V2f, Abc::IV2fProperty, iParent, header, iss);
                CASE_PRINT_VALUE(Abc::V2dTPTraits, Imath::V2d, Abc::IV2dProperty, iParent, header, iss);
                default:
                    std::cout << "Unknown property type" << std::endl;
                    break;
            }
        } else if ( extent == 3 ) {
            switch ( pod )
            {
                CASE_PRINT_VALUE(Abc::C3cTPTraits, Imath::C3c, Abc::IC3cProperty, iParent, header, iss);
                CASE_PRINT_VALUE(Abc::C3hTPTraits, Imath::C3h, Abc::IC3hProperty, iParent, header, iss);
                CASE_PRINT_VALUE(Abc::V3sTPTraits, Imath::V3s, Abc::IV3sProperty, iParent, header, iss);
                CASE_PRINT_VALUE(Abc::V3iTPTraits, Imath::V3i, Abc::IV3iProperty, iParent, header, iss);
                CASE_PRINT_VALUE(Abc::V3dTPTraits, Imath::V3d, Abc::IV3dProperty, iParent, header, iss);
                CASE_PRINT_VALUE(Abc::V3fTPTraits, Imath::V3f, Abc::IV3fProperty, iParent, header, iss);
                default:
                    std::cout << "Unknown property type" << std::endl;
                    break;
            }
        } /*else if ( extent == 6 ) {
            switch ( pod )
            {
                CASE_PRINT_VALUE(Abc::Box3sTPTraits, Imath::Box3s, Abc::IBox3sProperty, iParent, header, iss);
                CASE_PRINT_VALUE(Abc::Box3iTPTraits, Imath::Box3i, Abc::IBox3iProperty, iParent, header, iss);
                CASE_PRINT_VALUE(Abc::Box3fTPTraits, Imath::Box3f, Abc::IBox3fProperty, iParent, header, iss);
                CASE_PRINT_VALUE(Abc::Box3dTPTraits, Imath::Box3d, Abc::IBox3dProperty, iParent, header, iss);
                default:
                    std::cout << "Unknown property type" << std::endl;
                    break;
            }
        }*/
    }
}

//-*****************************************************************************
void printChild( Abc::ICompoundProperty iParent, Abc::PropertyHeader header,
                 bool all = false, bool long_list = false, bool meta = false )
{

    std::string ptype;
    AbcA::MetaData md;

    if ( long_list ) {
        std::stringstream ss;
        if ( header.isCompound() ) {
            ptype = "CompoundProperty";
            ss << "";
        } else if ( header.isScalar() ) {
            ptype = "ScalarProperty";
            ss << header.getDataType();
        } else if ( header.isArray() ) {
            ptype = "ArrayProperty";
            ss << header.getDataType();
        }
        std::cout << ptype
                  << std::string( COL_1 - ptype.length(), ' ' )
                  << ss.str()
                  << std::string( COL_2 - ss.str().length(), ' ' );
    }

    std::cout << BLUECOLOR << header.getName();

    if ( long_list ) {
        if ( header.isScalar() ) {
            Abc::IScalarProperty iProp( iParent, header.getName() );
            std::cout << "[" << iProp.getNumSamples() << "]";
        } else if ( header.isArray() ) {
            Abc::IArrayProperty iProp( iParent, header.getName() );
            std::cout << "[" << iProp.getNumSamples() << "]";
        }
        if ( meta ) {
            if ( header.isCompound() )
                getMetaData<Abc::ICompoundProperty>( iParent, header,
                                          all, long_list );
            else if ( header.isScalar() )
                getMetaData<Abc::IScalarProperty>( iParent, header,
                                          all, long_list );

            else if ( header.isArray() )
                getMetaData<Abc::IArrayProperty>( iParent, header,
                                          all, long_list );

        }
        std::cout << std::endl;
    }
    else
        std::cout << "   ";

    std::cout << RESETCOLOR;
}

//-*****************************************************************************
void printChild( AbcG::IObject iParent, AbcG::IObject iObj,
                 bool all = false, bool long_list = false, bool meta = false )
{

    AbcA::MetaData md = iObj.getMetaData();

    if ( long_list ) {
        std::string schema = md.get( "schema" );
        int spacing = COL_1;
        if ( all )
            spacing = COL_1 + COL_2;
        std::cout << schema << std::string(spacing - schema.length(), ' ');
    }
    std::cout << GREENCOLOR << iObj.getName();

    if ( meta )
         printMetaData( md, all, long_list );

    std::cout << RESETCOLOR;

    if ( long_list )
        std::cout << std::endl;
    else
        std::cout << "   ";
}

//-*****************************************************************************
void visit( Abc::ICompoundProperty iProp,
            bool all = false,
            bool long_list = false,
            bool meta = false,
            bool recursive = false,
            bool first = false )
{
    // header
    if ( recursive && iProp.getNumProperties() > 0 ) {
        printParent( iProp, all, long_list, recursive, first );
    }

    // children
    for( size_t i = 0; i < iProp.getNumProperties(); ++i ) {
        printChild( iProp, iProp.getPropertyHeader( i ), all, long_list, meta );
    }

    // visit children
    if ( recursive && all && iProp.getNumProperties() > 0 ) {
        for( size_t i = 0; i < iProp.getNumProperties(); ++i ) {
            Abc::PropertyHeader header = iProp.getPropertyHeader( i );
            if ( header.isCompound() )
                visit( Abc::ICompoundProperty( iProp, header.getName() ),
                       all, long_list, meta, recursive, false );
        }
    }
}

//-*****************************************************************************
void visit( AbcG::IObject iObj,
            bool all = false,
            bool long_list = false,
            bool meta = false,
            bool recursive = false,
            bool first = false )

{
    Abc::ICompoundProperty props = iObj.getProperties();

    // header
    if ( recursive &&
       ( iObj.getNumChildren() > 0 ||
       ( all && props.getNumProperties() > 0 ) ) ) {
        printParent( iObj, all, long_list, recursive, first);
    }

    // children
    for( size_t i = 0; i < iObj.getNumChildren(); ++i ) {
        printChild( iObj, iObj.getChild( i ), all, long_list, meta );
    }

    // properties
    if ( all ) {
        for( size_t i = 0; i < props.getNumProperties(); ++i ) {
            printChild( props, props.getPropertyHeader( i ), all, long_list, meta );
        }
    }

    // visit property children
    if ( recursive && all && props.getNumProperties() > 0 ) {
        for( size_t i = 0; i < props.getNumProperties(); ++i ) {
            Abc::PropertyHeader header = props.getPropertyHeader( i );
            if ( header.isCompound() ) {
                if ( !long_list )
                    std::cout << std::endl;
                visit( Abc::ICompoundProperty( props, header.getName() ),
                       all, long_list, meta, recursive, false );
            }
        }
    }

    // visit object children
    if ( recursive && iObj.getNumChildren() > 0 ) {
        for( size_t i = 0; i < iObj.getNumChildren(); ++i ) {
            visit( iObj.getChild( i ), all, long_list, meta, recursive, false );
        }
    }
}

//-*****************************************************************************
bool isFile( const std::string& filename )
{
    struct stat buf;
    if ( stat(filename.c_str(), &buf) == 0 && !S_ISDIR( buf.st_mode ) ) {
        return true;
    }
    return false;
}

//-*****************************************************************************
bool optionExists(std::vector<std::string> options, std::string option)
{
    for ( std::size_t i = 0; i < options.size(); i++ )
        if ( options[i].find(option) != std::string::npos )
            return true;
    return false;
}

//-*****************************************************************************
int main( int argc, char *argv[] )
{
    //float opt_fps = 24.0;
    bool opt_all = false; // show all option
    bool opt_long = false; // long listing option
    bool opt_meta = false; // metadata option
    bool opt_recursive = false; // recursive option
    int index = -1; // sample number, at tail of path
    std::string desc( "abcls [OPTION] FILE[/NAME] \n"
    "  -a          include property listings\n"
    //"  --fps[=FPS] show values in frames per second\n"
    "  -h, --help  show this help message\n"
    "  -l          long listing format\n"
    "  -m          show archive metadata\n"
    "  -r          list entries recursively\n"
    );

    // check for min args
    if ( argc < 2 ) {
        std::cout << desc << std::endl;
        return 0;
    };

    // parse args
    std::vector<std::string> arguments(argv, argv + argc);
    std::vector<std::string> options;
    std::vector<std::string> files;

    // separate file args from option args
    for ( std::size_t i = 1; i < arguments.size(); i++ ) {
        if ( arguments[ i ].substr( 0, 1 ) == "-" )
            options.push_back( arguments[ i ] );
        else
            files.push_back( arguments[ i ] );
    }

    // help
    if ( argc < 2 ||
         optionExists( options, "h" ) ||
         optionExists( options, "help" )
       ) {
        std::cout << desc << std::endl;
        return 0;
    };

    // set some flags
    opt_all = optionExists( options, "a" );
    opt_long = optionExists( options, "l" );
    opt_meta = optionExists( options, "m" );
    opt_recursive = optionExists( options, "r" );

    // open each file
    for ( std::size_t i = 0; i < files.size(); i++ ) {
        if ( files.size() > 1 )
            std::cout << BOLD << files[i] << ':' << RESETCOLOR << std::endl;

        std::stringstream ss( files[i] );
        std::stringstream fp;
        std::string segment;
        std::vector<std::string> seglist;

        /*
         * separate file and object paths, e.g.
         *
         *   ../dir1/foo.abc/bar/baz/index
         *   \_____________/\______/\____/
         *        file         obj   sample
         */
        int j = 0;
        while ( std::getline( ss, segment, '/' ) ) {
            if ( !isFile ( fp.str() ) ) {
                if ( j != 0 )
                    fp << "/";
                fp << segment;
            } else if ( is_digit( segment ) ) {
                index = atoi( segment.c_str() );
            } else {
                seglist.push_back( segment );
            }
            ++j;
        }

        // open the iarchive
        Abc::IArchive archive;
        AbcF::IFactory factory;
        factory.setPolicy(Abc::ErrorHandler::kQuietNoopPolicy);
        AbcF::IFactory::CoreType coreType;
        archive = factory.getArchive(std::string( fp.str() ), coreType);

        // display file metadata
        if ( opt_meta && seglist.size() == 0 ) {
            std::cout  << "Using "
                       << Alembic::AbcCoreAbstract::GetLibraryVersion()
                       << std::endl;;

            std::string appName;
            std::string libraryVersionString;
            Alembic::Util::uint32_t libraryVersion;
            std::string whenWritten;
            std::string userDescription;
            std::string coreName;
            GetArchiveInfo (archive,
                            appName,
                            libraryVersionString,
                            libraryVersion,
                            whenWritten,
                            userDescription);

            if ( coreType == AbcF::IFactory::kOgawa ) {
                coreName = "Ogawa";
            } else if ( coreType == AbcF::IFactory::kHDF5 ) {
                coreName = "HDF5";
            } else {
                coreName = "Unknown";
            };

            if ( appName != "" ) {
                std::cout << "  file written by: " << appName << std::endl;
                std::cout << "  using Alembic : " << libraryVersionString << std::endl;
                std::cout << "  written on : " << whenWritten << std::endl;
                std::cout << "  user description : " << userDescription << std::endl;
            } else {
                std::cout << "  (file doesn't have any ArchiveInfo)"
                          << std::endl;
            }
            std::cout << "  core type : " << coreName << std::endl;
        };

        // walk object hierarchy and find valid objects
        AbcG::IObject test = archive.getTop();
        AbcG::IObject iObj = test;
        while ( test.valid() && seglist.size() > 0 ) {
            test = test.getChild( seglist.front() );
            if ( test.valid() ) {
                iObj = test;
                seglist.erase( seglist.begin() );
            }
        }

        // walk property hierarchy for most recent object
        Abc::ICompoundProperty props = iObj.getProperties();
        const Abc::PropertyHeader* header;
        bool found = false;
        for ( std::size_t i = 0; i < seglist.size(); ++i ) {
            header = props.getPropertyHeader( seglist[i] );
            if ( header && header->isCompound() ) {
                Abc::ICompoundProperty ptest( props, header->getName() );
                if ( ptest.valid() ) {
                    props = ptest;
                    found = true;
                }
            } else if ( header && header->isSimple() ) {
                found = true;
            } else {
                std::cout << seglist[i]
                          << ": Invalid object or property"
                          << std::endl;
                return 1;
            }
        }

        // do stuff
        if ( index >= 0 ) {
            printValue( props, *header, index );

        } else {

            if ( found && header->isCompound() )
                visit( props, opt_all, opt_long, opt_meta, opt_recursive, true );
            else if ( found && header->isSimple() )
                printChild( props, *header, opt_all, opt_long );
            else
                visit( iObj, opt_all, opt_long, opt_meta, opt_recursive, true );

            std::cout << RESETCOLOR;
            if ( !opt_long )
                std::cout << std::endl;
        }
    }
    return 0;
}
