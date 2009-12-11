/*
 * Software License :
 *
 * Copyright (c) 2007-2009, The Open Effects Association Ltd. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * Neither the name The Open Effects Association Ltd, nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <assert.h>

#include <string>
#include <map>

// ofx
#include "ofxImageEffect.h"

// ofx host
#include "ofxhBinary.h"
#include "ofxhPropertySuite.h"
#include "ofxhClip.h"
#include "ofxhParam.h"
#include "ofxhMemory.h"
#include "ofxhImageEffect.h"
#include "ofxhPluginAPICache.h"
#include "ofxhPluginCache.h"
#include "ofxhHost.h"
#include "ofxhImageEffectAPI.h"
#include "ofxhXml.h"

#include <tuttle/host/core/Core.hpp>

// Disable the "this pointer used in base member initialiser list" warning in Windows
namespace tuttle {
namespace host {
namespace ofx {
namespace imageEffect {

#if defined( WINDOWS ) && !defined( __GNUC__ )
#pragma warning( disable : 4355 )
#endif

ImageEffectPlugin::ImageEffectPlugin( ImageEffectPluginCache& pc, PluginBinary* pb, int pi, OfxPlugin* pl )
	: Plugin( pb, pi, pl ),
	_pc( pc ),
	_baseDescriptor( NULL ),
	_pluginHandle( 0 )
{
	_baseDescriptor = core::Core::instance().getHost().makeDescriptor( this );
}

ImageEffectPlugin::ImageEffectPlugin( ImageEffectPluginCache& pc,
                                      PluginBinary*           pb,
                                      int                     pi,
                                      const std::string&      api,
                                      int                     apiVersion,
                                      const std::string&      pluginId,
                                      const std::string&      rawId,
                                      int                     pluginMajorVersion,
                                      int                     pluginMinorVersion )
	: Plugin( pb, pi, api, apiVersion, pluginId, rawId, pluginMajorVersion, pluginMinorVersion ),
	_pc( pc ),
	_baseDescriptor( NULL ),
	_pluginHandle( NULL )
{
	_baseDescriptor = core::Core::instance().getHost().makeDescriptor( this );
}

#if defined( WINDOWS ) && !defined( __GNUC__ )
#pragma warning( default : 4355 )
#endif

ImageEffectPlugin::~ImageEffectPlugin()
{
	for( std::map<std::string, Descriptor*>::iterator it = _contexts.begin(); it != _contexts.end(); ++it )
	{
		delete it->second;
	}
	_contexts.clear();
	if( getPluginHandle() )
	{
		OfxPlugin* op = _pluginHandle->getOfxPlugin();
		op->mainEntry( kOfxActionUnload, 0, 0, 0 );
	}
	delete _baseDescriptor;
}

APICache::PluginAPICacheI& ImageEffectPlugin::getApiHandler()
{
	return _pc;
}

/// get the image effect descriptor
Descriptor& ImageEffectPlugin::getDescriptor()
{
	return *_baseDescriptor;
}

/// get the image effect descriptor const version
const Descriptor& ImageEffectPlugin::getDescriptor() const
{
	return *_baseDescriptor;
}

void ImageEffectPlugin::addContext( const std::string& context, Descriptor* ied )
{
	_contexts[context] = ied;
	_knownContexts.insert( context );
}

void ImageEffectPlugin::addContext( const std::string& context )
{
	_knownContexts.insert( context );
}

void ImageEffectPlugin::saveXML( std::ostream& os )
{
	APICache::propertySetXMLWrite( os, getDescriptor().getProperties(), 6 );
}

const std::set<std::string>& ImageEffectPlugin::getContexts() const
{
	return _knownContexts;
}

bool ImageEffectPlugin::supportsContext( const std::string& context ) const
{
	/*
	std::cout << context << " supportsContext? " << _knownContexts.size() << std::endl;

	for( ContextSet::iterator it = _knownContexts.begin(),
			itEnd = _knownContexts.end();
			it != itEnd;
			++it )
	{
		TCOUT( "context " << *it );
	}
	*/

	return _knownContexts.find( context ) != _knownContexts.end();
}

void ImageEffectPlugin::initContexts()
{
	const tuttle::host::ofx::Property::Set& eProps = getDescriptor().getProperties();
	int size                                       = eProps.getDimension( kOfxImageEffectPropSupportedContexts );

	for( int j = 0; j < size; ++j )
	{
		std::string context = eProps.getStringProperty( kOfxImageEffectPropSupportedContexts, j );
		addContext( context );
	}
}

void ImageEffectPlugin::loadAndDescribeActions()
{
	if( getPluginHandle() )
	{
		//COUT( "loadAndDescribeAction already called on plugin " + getApiHandler()._apiName );
		return;
	}
	_pluginHandle.reset( new tuttle::host::ofx::PluginHandle( this, _pc.getHost() ) );

	OfxPlugin* op = _pluginHandle->getOfxPlugin();

	if( op == NULL )
	{
		COUT_ERROR( "loadAndDescribeAction OfxPlugin NULL, on plugin " + getApiHandler()._apiName );
		_pluginHandle.reset( NULL );
		return;
	}

	int rval = op->mainEntry( kOfxActionLoad, 0, 0, 0 );

	if( rval != kOfxStatOK && rval != kOfxStatReplyDefault )
	{
		_pluginHandle.reset( NULL );
		COUT_ERROR( "Load Action failed on plugin " + getApiHandler()._apiName );
		return;
	}

	rval = op->mainEntry( kOfxActionDescribe, getDescriptor().getHandle(), 0, 0 );

	if( rval != kOfxStatOK && rval != kOfxStatReplyDefault )
	{
		_pluginHandle.reset( NULL );
		COUT_ERROR( "Describe Action failed on plugin " + getApiHandler()._apiName );
		return;
	}
	initContexts();
}

Descriptor* ImageEffectPlugin::getDescriptorInContext( const std::string& context )
{
	std::map<std::string, Descriptor*>::iterator it = _contexts.find( context );

	//COUT( "context : " << context );
	if( it != _contexts.end() )
	{
		//COUT( "found context description : " << it->second->getLabel() );
		return it->second;
	}

	//	COUT( "ImageEffectPlugin::getContext -- _contexts" );
	//	for( std::map<std::string, Descriptor*>::const_iterator a = _contexts.begin(); a != _contexts.end(); ++a )
	//	{
	//		COUT( a->second->getLabel() );
	//	}
	//	COUT( "ImageEffectPlugin::getContext -- _knownContexts" );
	//	for( std::set<std::string>::const_iterator a = _knownContexts.begin(); a != _knownContexts.end(); ++a )
	//	{
	//		COUT( *a );
	//	}

	if( _knownContexts.find( context ) == _knownContexts.end() )
	{
		COUT_ERROR( "Context not found (" + context + ")" );
		return NULL;
	}
	return describeInContextAction( context );
}

Descriptor* ImageEffectPlugin::describeInContextAction( const std::string& context )
{
	tuttle::host::ofx::Property::PropSpec inargspec[] = {
		{ kOfxImageEffectPropContext, tuttle::host::ofx::Property::eString, 1, true, context.c_str() },
		{ 0 }
	};

	tuttle::host::ofx::Property::Set inarg( inargspec );

	PluginHandle* ph = getPluginHandle();

	std::auto_ptr<tuttle::host::ofx::imageEffect::Descriptor> newContext( core::Core::instance().getHost().makeDescriptor( getDescriptor(), this ) );
	int rval = kOfxStatFailed;
	if( ph->getOfxPlugin() )
		rval = ph->getOfxPlugin()->mainEntry( kOfxImageEffectActionDescribeInContext, newContext->getHandle(), inarg.getHandle(), 0 );

	if( rval == kOfxStatOK || rval == kOfxStatReplyDefault )
	{
		_contexts[context] = newContext.release();
		return _contexts[context];
	}
	COUT_ERROR( "kOfxImageEffectActionDescribeInContext failed (" << mapStatusToString( rval ) << ")" ); // Exception( rval )

	return NULL;
}

imageEffect::Instance* ImageEffectPlugin::createInstance( const std::string& context, void* clientData )
{
	/**
	 * @todo - we need to make sure action:load is called, then action:describe again
	 * (not because we are expecting the results to change, but because plugin
	 * might get confused otherwise), then a describe_in_context
	 */
	loadAndDescribeActions(); ///< @todo tuttle remove or not ? (already called from outside...)
	if( getPluginHandle() == NULL )
	{
		COUT_ERROR( "imageEffectPlugin::createInstance, unexpected error." );
		return NULL; // throw specific Exception
	}
	Descriptor* desc = getDescriptorInContext( context );
	if( !desc )
	{
		COUT_ERROR( "The plugin doesn't support the context " << context << "." );
		return NULL; // throw specific Exception
	}

	return core::Core::instance().getHost().newInstance( clientData, this, *desc, context );
}

void ImageEffectPlugin::unloadAction()
{
	if( getPluginHandle() )
	{
		( *_pluginHandle )->mainEntry( kOfxActionUnload, 0, 0, 0 );
	}
}

ImageEffectPluginCache::ImageEffectPluginCache( tuttle::host::ofx::imageEffect::ImageEffectHost& host )
	: PluginAPICacheI( kOfxImageEffectPluginApi, 1, 1 ),
	_currentPlugin( 0 ),
	_currentProp( 0 ),
	_currentContext( 0 ),
	_currentParam( 0 ),
	_currentClip( 0 ),
	_host( &host ) {}

ImageEffectPluginCache::~ImageEffectPluginCache() {}

/**
 * get the plugin by id.  vermaj and vermin can be specified.  if they are not it will
 * pick the highest found version.
 */
ImageEffectPlugin* ImageEffectPluginCache::getPluginById( const std::string& id, int vermaj, int vermin )
{
	// return the highest version one, which fits the pattern provided
	ImageEffectPlugin* sofar = 0;

	for( std::vector<ImageEffectPlugin*>::iterator i = _plugins.begin(); i != _plugins.end(); ++i )
	{
		ImageEffectPlugin* p = *i;

		if( p->getIdentifier() != id )
		{
			continue;
		}

		if( vermaj != -1 && p->getVersionMajor() != vermaj )
		{
			continue;
		}

		if( vermin != -1 && p->getVersionMinor() != vermin )
		{
			continue;
		}

		if( !sofar || p->trumps( sofar ) )
		{
			sofar = p;
		}
	}
	return sofar;
}

/// whether we support this plugin.
bool ImageEffectPluginCache::pluginSupported( tuttle::host::ofx::Plugin* p, std::string& reason ) const
{
	return core::Core::instance().getHost().pluginSupported( dynamic_cast<tuttle::host::ofx::imageEffect::ImageEffectPlugin*>( p ), reason );
}

/**
 * get the plugin by label. vermaj and vermin can be specified.  if they are not it will
 * pick the highest found version.
 */
ImageEffectPlugin* ImageEffectPluginCache::getPluginByLabel( const std::string& label, int vermaj, int vermin )
{
	// return the highest version one, which fits the pattern provided
	ImageEffectPlugin* sofar = 0;

	for( std::vector<ImageEffectPlugin*>::iterator i = _plugins.begin(); i != _plugins.end(); ++i )
	{
		ImageEffectPlugin* p = *i;

		if( p->getDescriptor().getProperties().getStringProperty( kOfxPropLabel ) != label )
		{
			continue;
		}

		if( vermaj != -1 && p->getVersionMajor() != vermaj )
		{
			continue;
		}

		if( vermin != -1 && p->getVersionMinor() != vermin )
		{
			continue;
		}

		if( !sofar || p->trumps( sofar ) )
		{
			sofar = p;
		}
	}

	return sofar;
}

const std::vector<ImageEffectPlugin*>& ImageEffectPluginCache::getPlugins() const
{
	return _plugins;
}

const std::map<std::string, ImageEffectPlugin*>& ImageEffectPluginCache::getPluginsByID() const
{
	return _pluginsByID;
}

/**
 * handle the case where the info needs filling in from the file.
 * Runs the "describe" action on the plugin.
 */
void ImageEffectPluginCache::loadFromPlugin( Plugin* op ) const
{
	std::string msg = "loading ";

	msg += op->getRawIdentifier();

	_host->loadingStatus( msg );

	ImageEffectPlugin* p = dynamic_cast<ImageEffectPlugin*>( op );
	assert( p );

	PluginHandle plug( p, _host );

	int rval = plug->mainEntry( kOfxActionLoad, 0, 0, 0 );

	if( rval != kOfxStatOK && rval != kOfxStatReplyDefault )
	{
		std::cerr << "load failed on plugin " << op->getIdentifier() << std::endl;
		return;
	}

	rval = plug->mainEntry( kOfxActionDescribe, p->getDescriptor().getHandle(), 0, 0 );

	if( rval != kOfxStatOK && rval != kOfxStatReplyDefault )
	{
		std::cerr << "describe failed on plugin " << op->getIdentifier() << std::endl;
		return;
	}

	const imageEffect::Descriptor& e = p->getDescriptor();
	const Property::Set& eProps      = e.getProperties();

	int size = eProps.getDimension( kOfxImageEffectPropSupportedContexts );

	for( int j = 0; j < size; ++j )
	{
		std::string context = eProps.getStringProperty( kOfxImageEffectPropSupportedContexts, j );
		p->addContext( context );
	}

	rval = plug->mainEntry( kOfxActionUnload, 0, 0, 0 );
}

/**
 * handler for preparing to read in a chunk of XML from the cache, set up context to do this
 */
void ImageEffectPluginCache::beginXmlParsing( Plugin* p )
{
	_currentPlugin = dynamic_cast<ImageEffectPlugin*>( p );
}

/**
 * XML handler : element begins (everything is stored in elements and attributes)
 */
void ImageEffectPluginCache::xmlElementBegin( const std::string& el, std::map<std::string, std::string> map )
{
	if( el == "apiproperties" )
	{
		return;
	}

	if( el == "context" )
	{
		_currentContext = core::Core::instance().getHost().makeDescriptor( _currentPlugin->getBinary()->getBundlePath(), _currentPlugin );
		_currentPlugin->addContext( map["name"], _currentContext );
		return;
	}

	if( el == "param" && _currentContext )
	{
		std::string pname = map["name"];
		std::string ptype = map["type"];

		_currentParam = _currentContext->paramDefine( ptype.c_str(), pname.c_str() );
		return;
	}

	if( el == "clip" && _currentContext )
	{
		std::string cname = map["name"];

		_currentClip = new attribute::ClipImageDescriptor( cname );
		_currentContext->addClip( cname, _currentClip );
		return;
	}

	if( _currentContext && _currentParam )
	{
		APICache::propertySetXMLRead( el, map, _currentParam->getEditableProperties(), _currentProp );
		return;
	}

	if( _currentContext && _currentClip )
	{
		APICache::propertySetXMLRead( el, map, _currentClip->getEditableProperties(), _currentProp );
		return;
	}

	if( !_currentContext && !_currentParam )
	{
		APICache::propertySetXMLRead( el, map, _currentPlugin->getDescriptor().getEditableProperties(), _currentProp );
		return;
	}

	std::cout << "element " << el << "\n";
	assert( false );
}

void ImageEffectPluginCache::xmlCharacterHandler( const std::string& ) {}

void ImageEffectPluginCache::xmlElementEnd( const std::string& el )
{
	if( el == "param" )
	{
		_currentParam = 0;
	}

	if( el == "context" )
	{
		_currentContext = 0;
	}
}

void ImageEffectPluginCache::endXmlParsing()
{
	_currentPlugin = 0;
}

void ImageEffectPluginCache::saveXML( Plugin* ip, std::ostream& os ) const
{
	ImageEffectPlugin* p = dynamic_cast<ImageEffectPlugin*>( ip );

	p->saveXML( os );
}

void ImageEffectPluginCache::confirmPlugin( Plugin* p )
{
	ImageEffectPlugin* plugin = dynamic_cast<ImageEffectPlugin*>( p );

	_plugins.push_back( plugin );

	if( _pluginsByID.find( plugin->getIdentifier() ) != _pluginsByID.end() )
	{
		ImageEffectPlugin* otherPlugin = _pluginsByID[plugin->getIdentifier()];
		if( plugin->trumps( otherPlugin ) )
		{
			_pluginsByID[plugin->getIdentifier()] = plugin;
		}
	}
	else
	{
		_pluginsByID[plugin->getIdentifier()] = plugin;
	}

	MajorPlugin maj( plugin );

	if( _pluginsByIDMajor.find( maj ) != _pluginsByIDMajor.end() )
	{
		ImageEffectPlugin* otherPlugin = _pluginsByIDMajor[maj];
		if( plugin->getVersionMajor() != otherPlugin->getVersionMajor() || plugin->trumps( otherPlugin ) )
		{
			_pluginsByIDMajor[maj] = plugin;
		}
	}
	else
	{
		_pluginsByIDMajor[maj] = plugin;
	}
}

Plugin* ImageEffectPluginCache::newPlugin( PluginBinary* pb,
                                           int           pi,
                                           OfxPlugin*    pl )
{
	ImageEffectPlugin* plugin = new ImageEffectPlugin( *this, pb, pi, pl );

	return plugin;
}

Plugin* ImageEffectPluginCache::newPlugin( PluginBinary*      pb,
                                           int                pi,
                                           const std::string& api,
                                           int                apiVersion,
                                           const std::string& pluginId,
                                           const std::string& rawId,
                                           int                pluginMajorVersion,
                                           int                pluginMinorVersion )
{
	ImageEffectPlugin* plugin = new ImageEffectPlugin( *this, pb, pi, api, apiVersion, pluginId, rawId, pluginMajorVersion, pluginMinorVersion );

	return plugin;
}

void ImageEffectPluginCache::dumpToStdOut() const
{
	if( _pluginsByID.empty() )
		std::cout << "No Plug-ins Found." << std::endl;

	std::cout << "________________________________________________________________________________" << std::endl;
	for( std::map<std::string, ImageEffectPlugin*>::const_iterator it = _pluginsByID.begin(); it != _pluginsByID.end(); ++it )
	{
		std::cout << "Plug-in:" << it->first << std::endl;
		std::cout << "\t" << "Filepath: " << it->second->getBinary()->getFilePath();
		std::cout << "(" << it->second->getIndex() << ")" << std::endl;

		std::cout << "Contexts:" << std::endl;
		const std::set<std::string>& contexts = it->second->getContexts();
		for( std::set<std::string>::const_iterator it2 = contexts.begin(); it2 != contexts.end(); ++it2 )
			std::cout << "\t* " << *it2 << std::endl;
		const Descriptor& d = it->second->getDescriptor();
		std::cout << "Inputs:" << std::endl;
		const std::map<std::string, attribute::ClipImageDescriptor*>& inputs = d.getClips();
		for( std::map<std::string, attribute::ClipImageDescriptor*>::const_iterator it2 = inputs.begin(); it2 != inputs.end(); ++it2 )
			std::cout << "\t\t* " << it2->first << std::endl;
		std::cout << "________________________________________________________________________________" << std::endl;
	}
}

}
}
}
}

