#include <tuttle/common/exceptions.hpp>
#include <tuttle/host/Graph.hpp>

#include <boost/program_options.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/foreach.hpp>


#ifndef SAMDO_PIPE_STR
#define SAMDO_PIPE_STR "//"
#endif

static const std::string kpipe = SAMDO_PIPE_STR;

struct NodeCommand
{
	NodeCommand( const std::vector<std::string>& commandLine )
	: _name( commandLine[0] )
	{
		BOOST_ASSERT( commandLine.size() );
	}

	std::string _name;
	std::vector< std::pair<std::string, std::string> > _params;
	std::vector< std::pair<std::string, std::string> > _flags;
};

int main( int argc, char** argv )
{
	try
	{
		if( argc <= 1 ) // no argument
		{
			TUTTLE_COUT(
				"sam-do: missing operands.\n"
				"'sam-do --help' for more informations.\n"
			);
			exit( -1 );
		}
		
		std::vector<std::string> cl_options;
		std::vector< std::vector<std::string> > cl_commands;
		cl_commands.reserve(10);
		cl_commands.resize(1); // first node for options

		// split the command line to identify the multiple parts
		for( int i = 1; i < argc; ++i )
		{
			std::string s( argv[i] );
			if( s == kpipe )
			{
				cl_commands.resize( cl_commands.size()+1 );
				cl_commands.back().reserve(10);
				continue;
			}
			cl_commands.back().push_back( s );
		}

		// reoganize nodes
		// first and last nodes may only be flags to sam-do itself
		{
			// First node
			std::vector<std::string>& firstNode = cl_commands.front();
			std::vector<std::string>::iterator fNodeIt = firstNode.begin();
			std::vector<std::string>::iterator fNodeItEnd = firstNode.end();
			while( fNodeIt != fNodeItEnd &&
				   (*fNodeIt)[0] == '-' )
			{
				++fNodeIt;
			}
			// options in front of the first node are options for sam-do
			if( firstNode.begin() != fNodeIt )
			{
				std::copy( firstNode.begin(), fNodeIt, std::back_inserter(cl_options) );
				firstNode.erase( firstNode.begin(), fNodeIt );
			}
			if( firstNode.size() == 0 )
			{
				cl_commands.erase( cl_commands.begin() );
			}
		}
		if( cl_commands.size() )
		{
			// Last node
			// Is it a node or only flags?
			const std::vector<std::string>& notNode = cl_commands.back();
			if( notNode[0][0] == '-' )
			{
				std::copy( notNode.begin(), notNode.end(), std::back_inserter(cl_options) );
				cl_commands.erase( cl_commands.end()-1 );
			}
		}

		namespace ttl = tuttle::host;

		ttl::Core::instance().preload();
		const std::vector<ttl::ofx::imageEffect::OfxhImageEffectPlugin*> allNodes = ttl::Core::instance().getImageEffectPluginCache().getPlugins();

		// create the graph
		ttl::Graph graph;
		std::list<ttl::Graph::Node*> nodes;
		std::vector<std::ssize_t> range;
		std::size_t step;
		
		// Analyze each part of the command line
		{
			namespace bpo = boost::program_options;
			// Analyze sam-do flags
			try
			{
				// Declare the supported options.
				bpo::options_description infoOptions( "\tDisplay options (replace the process)" );
				infoOptions.add_options()
					("help,h"       , "show node help")
					("version,v"    , "display node version")
					("nodes,n"      , "show list of all available nodes")
				;
				bpo::options_description confOptions( "\tConfigure process" );
				confOptions.add_options()
					("range,r"      , bpo::value< std::vector<std::ssize_t> >(), "frame range to render" )
					("verbose,V"    , "explain what is being done")
					("quiet,Q"      , "don't print commands")
					("nb-cores"     , bpo::value<std::string>(), "set a fix number of CPUs")
				;

				bpo::options_description all_options;
				all_options.add(infoOptions).add(confOptions);

				bpo::variables_map samdo_vm;
				bpo::store( bpo::command_line_parser(cl_options).options(all_options).run(), samdo_vm );
				if( const char* env_ptr = std::getenv("SAM_DO_OPTIONS") )
				{
					std::vector<std::string> envOptions;
					std::string env( env_ptr );
					envOptions.push_back( env );
					bpo::store( bpo::command_line_parser(envOptions).options(all_options).run(), samdo_vm );
				}

				bpo::notify( samdo_vm );

				if( samdo_vm.count("help") )
				{
					TUTTLE_COUT( "TuttleOFX project [http://sites.google.com/site/tuttleofx]" );
					TUTTLE_COUT( "" );
					TUTTLE_COUT( "NAME" );
					TUTTLE_COUT( "\tsam-do - A command line to execute a list of OpenFX nodes." );
					TUTTLE_COUT( "\t         Use the sperarator // to pipe images between nodes." );
					TUTTLE_COUT( "" );
					TUTTLE_COUT( "SYNOPSIS" );
					TUTTLE_COUT( "\tsam-do [options]... [// node [node-options]... [[param=]value]...]... [// [options]...]" );
					TUTTLE_COUT( "" );
					TUTTLE_COUT( "EXAMPLES" );
					TUTTLE_COUT( "\tsam-do r foo.####.dpx // w foo.####.jpg" );
					TUTTLE_COUT( "\tsam-do --nodes" );
					TUTTLE_COUT( "\tsam-do blur -h" );
					TUTTLE_COUT( "\tsam-do --verbose read foo.####.dpx // blur 3 // resize scale=50% // write foo.####.jpg range=[10,20]" );
					TUTTLE_COUT( "\tsam-do r foo.dpx // sobel // print // -Q" );
					TUTTLE_COUT( "" );
					TUTTLE_COUT( "DESCRIPTION" );
					TUTTLE_COUT( infoOptions );
					TUTTLE_COUT( confOptions );
					exit( 0 );
				}
				if( samdo_vm.count("version") )
				{
					TUTTLE_COUT( "TuttleOFX Host - version " << TUTTLE_HOST_VERSION_STR );
					exit( 0 );
				}
				if( samdo_vm.count("nodes") )
				{
					TUTTLE_COUT( "NODES" );
					for( std::size_t i = 0; i < allNodes.size(); ++i )
					{
						const std::string plugName = allNodes.at(i)->getRawIdentifier();

						std::vector< std::string > termsPlugin;
						boost::algorithm::split( termsPlugin, plugName, boost::algorithm::is_any_of("."));

						TUTTLE_COUT( "\t" << termsPlugin.back() );
					}
					exit( 0 );
				}
				{
					std::vector<std::ssize_t> vmrange;
					if( samdo_vm.count("range") )
						vmrange = samdo_vm["range"].as<std::vector<std::ssize_t> >();
					
					if( vmrange.size() >= 1 )
						range.push_back( vmrange[0] );
					else
						range.push_back( 0 );
					
					if( vmrange.size() >= 2 )
						range.push_back( vmrange[1] );
					else
						range.push_back( range[0] );
					
					if( vmrange.size() >= 3 )
						step = vmrange[2];
					else
						step = 1;
				}
			}
			catch( const boost::program_options::error& e )
			{
				TUTTLE_CERR( "sam-do options" );
				TUTTLE_CERR( "Error: " << e.what() );
				exit( -2 );
			}
			catch( ... )
			{
				TUTTLE_CERR( "sam-do options" );
				TUTTLE_CERR( "Error: " << boost::current_exception_diagnostic_information() );
				exit( -2 );
			}

			/// @todo Set all sam-do options for rendering

			// Analyse options for each node
			{
				// Declare the supported options.
				bpo::options_description infoOptions( "\tDisplay options (replace the process)" );
				infoOptions.add_options()
					("help,h"       , "show node help")
					("version,v"    , "display node version")
					("attributes,a" , "show all attributes: parameters+clips")
					("properties,p" , "list properties of the node")
					("clips,c"      , "list clips of the node")
					("clip"         , bpo::value<std::string>(), "display clip informations")
					("parameters,P" , "list parameters of the node")
					("param"        , bpo::value<std::string>(), "display parameter informations")
				;
				bpo::options_description confOptions( "\tConfigure process" );
				confOptions.add_options()
					("verbose,V"    , "explain what is being done")
					("nb-cores"     , bpo::value<std::string>(), "set a fix number of CPUs")
				;
				// describe hidden options
				bpo::options_description hiddenOptions;
				hiddenOptions.add_options()
					("param-values", bpo::value< std::vector<std::string> >(), "node parameters")
				;

				// define default options
				bpo::positional_options_description param_values;
				param_values.add("param-values", -1);

				bpo::options_description all_options;
				all_options.add(infoOptions).add(confOptions).add(hiddenOptions);
				
				BOOST_FOREACH( const std::vector<std::string>& command, cl_commands )
				{
					std::string userNodeName = command[0];
					std::string nodeFullName = command[0];
//					boost::algorithm::to_lower( userNodeName );
					std::vector<std::string> nodeArgs;
					std::copy( command.begin()+1, command.end(), std::back_inserter(nodeArgs) );
					
					try
					{
						// parse the command line, and put the result in node_vm
						bpo::variables_map node_vm;
						bpo::store( bpo::command_line_parser(nodeArgs).options(all_options).positional(param_values).run(), node_vm );

						bpo::notify( node_vm );

						std::vector< std::string > detectedPlugins;
						for( std::size_t i = 0; i < allNodes.size(); ++i )
						{
							const std::string plugName = allNodes.at(i)->getRawIdentifier();
							if( plugName == userNodeName )
							{
								detectedPlugins.clear();
								detectedPlugins.push_back( plugName );
								break;
							}
							if( boost::algorithm::find_first(plugName, userNodeName ) )
							{
								detectedPlugins.push_back( plugName );
							}
						}
						if( detectedPlugins.size() != 1 )
						{
							if( detectedPlugins.size() < 1 )
							{
								BOOST_THROW_EXCEPTION( tuttle::exception::Value()
									<< tuttle::exception::user() + "Unrecognized node name \"" + userNodeName + "\"." );
							}
							else
							{
								tuttle::exception::user userMsg;
								userMsg + "Ambiguous node name \"" + userNodeName + "\".\n";
								userMsg + "Possible nodes:\n";
								BOOST_FOREACH( const std::string& p, detectedPlugins )
								{
									userMsg + " - \"" + p + "\"\n";
								}
								BOOST_THROW_EXCEPTION( tuttle::exception::Value()
									<< userMsg );
							}
						}
						nodeFullName = detectedPlugins.front();

						TUTTLE_COUT( "[" << nodeFullName << "]" );

						ttl::Graph::Node& currentNode = graph.createNode( nodeFullName );
						nodes.push_back( &currentNode );
						
						// Check priority flags:
						// If one flag to display informations is used in command line,
						// it replaces all the process.
						// --help,h --version,v --verbose,V --params --clips --props
						
						if( node_vm.count("help") )
						{
							TUTTLE_COUT( "TuttleOFX project [http://sites.google.com/site/tuttleofx]" );
							TUTTLE_COUT( "" );
							TUTTLE_COUT( "NODE" );
							TUTTLE_COUT( "\tsam-do " << nodeFullName << " - OpenFX node." );
							TUTTLE_COUT( "" );
							TUTTLE_COUT( "DESCRIPTION" );
							TUTTLE_COUT( "\tnode type: " << ttl::mapNodeTypeEnumToString( currentNode.getNodeType() ) );
							TUTTLE_COUT( "" );
							// internal node help
							if( currentNode.getProperties().hasProperty( kOfxPropPluginDescription ) )
							{
								TUTTLE_COUT( currentNode.getProperties().fetchStringProperty( kOfxPropPluginDescription ).getValue(0) );
							}
							else if( currentNode.getNodeType() == ttl::INode::eNodeTypeImageEffect &&
							         currentNode.asImageEffectNode().getDescriptor().getProperties().hasProperty( kOfxPropPluginDescription ) )
							{
								TUTTLE_COUT( "\tDescriptor:" );
								TUTTLE_COUT( currentNode.asImageEffectNode().getDescriptor().getProperties().fetchStringProperty( kOfxPropPluginDescription ).getValue(0) );
							}
							else
							{
								TUTTLE_COUT( "\tNo description." );
							}
							TUTTLE_COUT( "" );
							TUTTLE_COUT( infoOptions );
							TUTTLE_COUT( confOptions );
							exit(0);
						}
						if( node_vm.count("version") )
						{
							TUTTLE_COUT( "\tsam-do " << nodeFullName );
							TUTTLE_COUT( "Version " << currentNode.getVersionStr() );
							exit(0);
						}
						if( node_vm.count("attributes") )
						{
							TUTTLE_COUT( "\tsam-do " << nodeFullName );
							TUTTLE_COUT( "ATTRIBUTES" );
							TUTTLE_COUT( "\tCLIPS" );
							/// @todo
							TUTTLE_COUT( "\tPARAMETERS" );
							/// @todo
							exit(0);
						}
						if( node_vm.count("properties") )
						{
							TUTTLE_COUT( "\tsam-do " << nodeFullName );
							TUTTLE_COUT( "PROPERTIES" );
							/// @todo
							exit(0);
						}
						if( node_vm.count("clips") )
						{
							TUTTLE_COUT( "\tsam-do " << nodeFullName );
							TUTTLE_COUT( "CLIPS" );
							/// @todo
							exit(0);
						}
						if( node_vm.count("clip") )
						{
							TUTTLE_COUT( "\tsam-do " << nodeFullName );
							TUTTLE_COUT( "CLIP: " << node_vm["clip"].as<std::string>() );
							/// @todo
							exit(0);
						}
						if( node_vm.count("parameters") )
						{
							TUTTLE_COUT( "\tsam-do " << nodeFullName );
							TUTTLE_COUT( "PARAMETERS" );
							/// @todo
							exit(0);
						}
						if( node_vm.count("param") )
						{
							TUTTLE_COUT( "\tsam-do " << nodeFullName );
							TUTTLE_COUT( "PARAM: " << node_vm["clip"].as<std::string>() );
							/// @todo
							exit(0);
						}

						// Analyse parameters
						static const boost::regex re_param( "(?:([a-zA-Z_][a-zA-Z0-9_]*)=)?(.*)" );
						if( node_vm.count("param-values") )
						{
							bool orderedParams = true;
							std::size_t paramIdx = 0;
							const std::vector<std::string> params = node_vm["param-values"].as< std::vector<std::string> >();
							BOOST_FOREACH( const std::string& p, params )
							{
								boost::cmatch matches;
								if( ! boost::regex_match( p.c_str(), matches, re_param ) )
								{
									BOOST_THROW_EXCEPTION( tuttle::exception::Value()
										<< tuttle::exception::user() + "Parameter can't be parsed \"" + p + "\"." );
								}
								if( matches.size() != 3 )
								{
									// should never happen
									BOOST_THROW_EXCEPTION( tuttle::exception::Value()
										<< tuttle::exception::user() + "Parameter can't be parsed \"" + p + "\". " + matches.size() + " matches." );
								}
								const std::string paramName = matches[1];
								const std::string paramValue = matches[2];
								if( paramName.size() )
								{
									orderedParams = false;
								}
								else if( orderedParams == false )
								{
									BOOST_THROW_EXCEPTION( tuttle::exception::Value()
										<< tuttle::exception::user() + "Non-keyword parameter after keyword parameter. \"" + p + "\"." );
								}
//								TUTTLE_COUT( "* " << p );
//								TUTTLE_COUT( "3: " << paramName << " => " << paramValue );

								/// @todo setup the node with parameter value in tuttle.
								if( paramName.size() )
								{
									currentNode.getParam( paramName ).setValueFromExpression( paramValue );
								}
								else
								{
									currentNode.getParam( paramIdx ).setValueFromExpression( paramValue );
								}
								++paramIdx;
							}
						}
					}
					catch( const tuttle::exception::Common& e )
					{
						TUTTLE_CERR( "sam-do - " << nodeFullName );
						TUTTLE_CERR( "Error: " << *boost::get_error_info<tuttle::exception::user>(e) );
						TUTTLE_CERR( "\n" );
						TUTTLE_CERR( "Debug: " << boost::current_exception_diagnostic_information() );
						exit( -2 );
					}
					catch( const boost::program_options::error& e )
					{
						TUTTLE_CERR( "sam-do - " << nodeFullName );
						TUTTLE_CERR( "Error: " << e.what() );
						TUTTLE_CERR( "\n" );
						TUTTLE_CERR( "Debug: " << boost::current_exception_diagnostic_information() );
						exit( -2 );
					}
					catch( ... )
					{
						TUTTLE_CERR( "sam-do - " << nodeFullName );
						TUTTLE_CERR( "Unknown error." );
						TUTTLE_CERR( "\n" );
						TUTTLE_CERR( "Debug: " << boost::current_exception_diagnostic_information() );
						exit( -2 );
					}
				}
			}
		}

		// display nodes
//		BOOST_FOREACH( const std::string& option, cl_options )
//		{
//			TUTTLE_COUT( "| " << option );
//		}
//		BOOST_FOREACH( const std::vector<std::string>& node, cl_commands )
//		{
//			TUTTLE_COUT( "[" << node[0] << "]" );
//			for( std::size_t i = 1; i < node.size(); ++i )
//			{
//				const std::string& s = node[i];
//				if( s[0] == '-' )
//				{
//					TUTTLE_COUT( s );
//				}
//				else
//				{
//					TUTTLE_COUT( "* " << s );
//				}
//			}
//		}

		// Connect all nodes linearly
		graph.connect( nodes );

		// Execute the graph
		graph.compute( *nodes.back(), range[0], range[1] );
	}
	catch( const tuttle::exception::Common& e )
	{
		TUTTLE_CERR( "sam-do" );
		TUTTLE_CERR( "Error: " << *boost::get_error_info<tuttle::exception::user>(e) );
		exit( -2 );
	}
	catch( const boost::program_options::error& e )
	{
		TUTTLE_CERR( "sam-do");
		TUTTLE_CERR( "Error: " << e.what() );
		exit( -2 );
	}
	catch( ... )
	{
		TUTTLE_CERR( "sam-do");
		TUTTLE_CERR( boost::current_exception_diagnostic_information() );
		exit( -2 );
	}
	return 0;
}

