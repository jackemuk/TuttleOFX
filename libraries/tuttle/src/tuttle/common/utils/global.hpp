#ifndef _TUTTLE_COMMON_UTILS_GLOBAL_HPP_
#define _TUTTLE_COMMON_UTILS_GLOBAL_HPP_

#include "color.hpp"

////////////////////////////////////////////////////////////////////////////////
// System stuff
#include <tuttle/common/system/system.hpp>
#include <tuttle/common/system/compatibility.hpp>
#include <tuttle/common/system/windows/windows.h>

////////////////////////////////////////////////////////////////////////////////
// Assert needs to be everywhere
#include <boost/static_assert.hpp>
#include <boost/type_traits.hpp>
#include <boost/current_function.hpp>

#include <boost/log/trivial.hpp>

////////////////////////////////////////////////////////////////////////////////
// Define functions to display infos in the console
#include <iostream>

#ifdef NDEBUG
#  if defined( _MSC_VER )
#    define TUTTLE_FORCEINLINE __forceinline
#  elif defined( __GNUC__ ) && __GNUC__ > 3
#    define TUTTLE_FORCEINLINE inline __attribute__ ( ( always_inline ) )
#  else
#    define TUTTLE_FORCEINLINE inline
#  endif
#else
#  define TUTTLE_FORCEINLINE inline
#endif

#define TUTTLE_TRACE BOOST_LOG_TRIVIAL(trace)
#define TUTTLE_INFO BOOST_LOG_TRIVIAL(info)
#define TUTTLE_WARNING BOOST_LOG_TRIVIAL(warning)
#define TUTTLE_ERROR BOOST_LOG_TRIVIAL(error)
#define TUTTLE_FATAL BOOST_LOG_TRIVIAL(fatal)

/**
 * @def   TUTTLE_INFOS
 * @brief informations : filename, line number, function name
 **/

#define TUTTLE_GET_INFOS_FILE      "in file:  " << __FILE__ << ",  line: " << __LINE__
#define TUTTLE_GET_INFOS_FUNCTION  "function: " << BOOST_CURRENT_FUNCTION
#define TUTTLE_GET_INFOS           TUTTLE_GET_INFOS_FILE << TUTTLE_GET_INFOS_FUNCTION

#define TUTTLE_GET_VAR( a )           #a << ": " << a
#define TUTTLE_GET_VAR2( a, b )       TUTTLE_GET_VAR ( a ) << ", " << TUTTLE_GET_VAR ( b )
#define TUTTLE_GET_VAR3( a, b, c )    TUTTLE_GET_VAR ( a ) << ", " << TUTTLE_GET_VAR ( b ) << ", " << TUTTLE_GET_VAR ( c )
#define TUTTLE_GET_VAR4( a, b, c, d ) TUTTLE_GET_VAR ( a ) << ", " << TUTTLE_GET_VAR ( b ) << ", " << TUTTLE_GET_VAR ( c ) << ", " << TUTTLE_GET_VAR ( d )


/**
 * @param[in] ... : all parameters with an operator << defined
 * @brief terminal display
 **/

#define TUTTLE_LOG_TRACE( ... )   BOOST_LOG_TRIVIAL(trace) << __VA_ARGS__
#define TUTTLE_LOG_INFO( ... )    BOOST_LOG_TRIVIAL(info)  << __VA_ARGS__
#define TUTTLE_LOG_WARNING( ... ) BOOST_LOG_TRIVIAL(warning) << common::Color::get()->_yellow << "warning: " << __VA_ARGS__ << common::Color::get()->_std
#define TUTTLE_LOG_ERROR( ... )   BOOST_LOG_TRIVIAL(error)   << common::Color::get()->_error  << "error: "   << __VA_ARGS__ << common::Color::get()->_std
#define TUTTLE_LOG_FATAL( ... )   BOOST_LOG_TRIVIAL(fatal)   << common::Color::get()->_error  << "fatal: "   << __VA_ARGS__ << common::Color::get()->_std

#define TUTTLE_LOG( MODE, ... ) MODE << __VA_ARGS__

#define TUTTLE_LOG_VAR( MODE, a )           TUTTLE_LOG( MODE, TUTTLE_GET_VAR ( a ) )
#define TUTTLE_LOG_VAR2( MODE, a, b )       TUTTLE_LOG( MODE, TUTTLE_GET_VAR2( a, b ) )
#define TUTTLE_LOG_VAR3( MODE, a, b, c )    TUTTLE_LOG( MODE, TUTTLE_GET_VAR3( a, b, c ) )
#define TUTTLE_LOG_VAR4( MODE, a, b, c, d ) TUTTLE_LOG( MODE, TUTTLE_GET_VAR4( a, b, c, d ) )

/**
 * @brief terminal information display
 **/
 #define TUTTLE_LOG_INFOS TUTTLE_LOG_TRACE( TUTTLE_GET_INFOS_FILE ); TUTTLE_LOG_TRACE( TUTTLE_GET_INFOS_FUNCTION )

/**
 * @param[in] ... : all parameters with an operator << defined
 * @brief terminal information display
 **/
/*
 #define TUTTLE_COUT_WITHINFOS(... )  \
    TUTTLE_TRACE( TUTTLE_INFOS_FILE ); \
    TUTTLE_TRACE( TUTTLE_INFOS_FUNCTION ); \
    TUTTLE_COUT( __VA_ARGS__ )

 #define TUTTLE_COUT_WARNING(... )  \
    TUTTLE_CERR( "TuttleOFX - Warning:" ); \
    TUTTLE_CERR( TUTTLE_INFOS_FILE ); \
    TUTTLE_CERR( TUTTLE_INFOS_FUNCTION ); \
    TUTTLE_CERR( __VA_ARGS__ )

 #define TUTTLE_COUT_ERROR(... )  \
    TUTTLE_CERR( tuttle::common::kColorError << "TuttleOFX - Error:" ); \
    TUTTLE_CERR( TUTTLE_INFOS_FILE ); \
    TUTTLE_CERR( TUTTLE_INFOS_FUNCTION ); \
    TUTTLE_CERR( __VA_ARGS__ << tuttle::common::kColorStd )

 #define TUTTLE_COUT_FATALERROR(... )  \
    TUTTLE_CERR( tuttle::common::kColorError << "TuttleOFX - Fatal error:" ); \
    TUTTLE_CERR( TUTTLE_INFOS_FILE ); \
    TUTTLE_CERR( TUTTLE_INFOS_FUNCTION ); \
    TUTTLE_CERR( __VA_ARGS__ << tuttle::common::kColorStd )
*/

////////////////////////////////////////////////////////////////////////////////
// Some specifics things to debug or release version
#ifdef DEBUG
 #include "debug.hpp"
#else
 #include "release.hpp"
#endif

////////////////////////////////////////////////////////////////////////////////
// TUTTLE_TLOG* defines are used by developers for temporary displays during development stages.
// They are removed in production mode.
#ifndef TUTTLE_PRODUCTION
	#define TUTTLE_TLOG TUTTLE_LOG
	#define TUTTLE_TLOG_VAR TUTTLE_LOG_VAR
	#define TUTTLE_TLOG_VAR2 TUTTLE_LOG_VAR2
	#define TUTTLE_TLOG_VAR3 TUTTLE_LOG_VAR3
	#define TUTTLE_TLOG_VAR4 TUTTLE_LOG_VAR4
	#define TUTTLE_TLOG_INFOS TUTTLE_LOG_INFOS
	#define TUTTLE_TLOG_WITHINFOS TUTTLE_LOG_WITHINFOS
	#define TUTTLE_TLOG_EXCEPTION TUTTLE_LOG_EXCEPTION
#else
	#define TUTTLE_TLOG TUTTLE_LOG_DEBUG
	#define TUTTLE_TLOG_VAR TUTTLE_LOG_VAR_DEBUG
	#define TUTTLE_TLOG_VAR2 TUTTLE_LOG_VAR2_DEBUG
	#define TUTTLE_TLOG_VAR3 TUTTLE_LOG_VAR3_DEBUG
	#define TUTTLE_TLOG_VAR4 TUTTLE_LOG_VAR4_DEBUG
	#define TUTTLE_TLOG_INFOS TUTTLE_LOG_INFOS_DEBUG
	#define TUTTLE_TLOG_WITHINFOS TUTTLE_LOG_WITHINFOS_DEBUG
	#define TUTTLE_TLOG_EXCEPTION TUTTLE_LOG_EXCEPTION_DEBUG
#endif

#define TUTTLE_LOG_PLUGIN_NAME_WIDTH 30

#endif
