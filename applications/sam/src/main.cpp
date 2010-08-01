
#include <tuttle/host/core/Graph.hpp>


void sam_terminate(void)
{
	std::cerr << "Sorry, Sam has encountered a fatal error." << std::endl;
	std::cerr << "Please report this bug." << std::endl;
	exit(-1);
}

void sam_unexpected(void)
{
	std::cerr << "Sorry, Sam has encountered an unexpected exception." << std::endl;
	std::cerr << "Please report this bug." << std::endl;
	BOOST_THROW_EXCEPTION( std::runtime_error( "Sorry, Sam has encountered an unexpected exception.\nPlease report this bug." ) );
}


int main( int argc, char** argv )
{
	std::set_terminate( &sam_terminate );
	std::set_unexpected( &sam_unexpected );
	try
	{
		using namespace tuttle::host;
		COUT( "__________________________________________________0" );		
		//core::Core::instance().getPluginCache().addDirectoryToPath( "/path/to/plugins" );
		//core::Core::instance().getPluginCache().scanPluginFiles();
		core::Core::instance().preload();
		
		COUT( core::Core::instance().getImageEffectPluginCache() );

		COUT( "__________________________________________________1" );

		core::Graph g;
//		core::Graph::Node& read1   = g.createNode( "fr.tuttle.ffmpegreader" );
		core::Graph::Node& read1   = g.createNode( "fr.tuttle.pngreader" );
		core::Graph::Node& read2   = g.createNode( "fr.tuttle.dpxreader" );
		core::Graph::Node& read3   = g.createNode( "fr.tuttle.exrreader" );
		core::Graph::Node& invert1 = g.createNode( "fr.tuttle.invert" );
//		core::Graph::Node& invert2 = g.createNode( "fr.tuttle.invert" );
		core::Graph::Node& invert2 = g.createNode( "fr.tuttle.imagestatistics" );
		core::Graph::Node& blur1 = g.createNode( "fr.tuttle.blur" );
		core::Graph::Node& invert4 = g.createNode( "fr.tuttle.invert" );
	//	core::Graph::Node& crop1   = g.createNode( "fr.tuttle.crop" );
		core::Graph::Node& merge1  = g.createNode( "fr.tuttle.merge" );
		core::Graph::Node& bitdepth1  = g.createNode( "fr.tuttle.bitdepth" );
		core::Graph::Node& write1  = g.createNode( "fr.tuttle.pngwriter" );
		core::Graph::Node& write2  = g.createNode( "fr.tuttle.dpxwriter" );
		core::Graph::Node& write3  = g.createNode( "fr.tuttle.exrwriter" );
		core::Graph::Node& write4  = g.createNode( "fr.tuttle.ffmpegwriter" );

		COUT( "__________________________________________________2" );
		// Setup parameters
//		read1.getParam( "filename" ).set( "data/input1.avi" );
		read1.getParam( "filename" ).set( "data/input.png" );
		read2.getParam( "filename" ).set( "data/input.dpx" );
		read3.getParam( "filename" ).set( "data/input.exr" );
//		blur1.getParam( "size" ).set( 6.5, 15.0 );
		blur1.getParam( "size" ).setAtIndex( 65.0, 1 );
	//	crop1.getParam( "Down" ).set( 400 );
		write1.getParam( "filename" ).set( "data/output1.png" );
		write2.getParam( "filename" ).set( "data/output2.dpx" );
		write3.getParam( "filename" ).set( "data/output3.exr" );
		write4.getParam( "filename" ).set( "data/output4.avi" );

		COUT( "__________________________________________________3" );
		g.connect( read1, invert1 );
		g.connect( invert1, invert2 );
		g.connect( invert2, blur1 );
		g.connect( blur1, write1 );
		g.connect( invert1, invert4 );
		g.connect( invert4, write2 );
		g.connect( invert1, write3 );

		g.connect( invert1, bitdepth1);
		g.connect( bitdepth1, merge1.getProcessAttribute("SourceA") );
		g.connect( read3, merge1.getProcessAttribute("SourceB") );
	//	g.connect( merge1, crop1 );
		g.connect( merge1, write4 );

		COUT( "__________________________________________________4" );
		std::list<std::string> outputs;
		outputs.push_back( write1.getName() );
		outputs.push_back( write2.getName() );
		outputs.push_back( write3.getName() );
		outputs.push_back( write4.getName() );
		g.compute( write1.getName(), 0, 0 );
	}
	catch( tuttle::host::core::exception::LogicError& e )
	{
		std::cout << "Tuttle Exception : main de sam." << std::endl;
		std::cerr << boost::diagnostic_information(e);
	}
	catch(... )
	{
		std::cerr << "Exception ... : main de sam." << std::endl;
		std::cerr << boost::current_exception_diagnostic_information();

	}

	return 0;
}

