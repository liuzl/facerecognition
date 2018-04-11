// The contents of this file are in the public domain. See LICENSE_FOR_EXAMPLE_PROGRAMS.txt
/*

    This example illustrates the use of the HTTP extension to the server object 
    from the dlib C++ Library.
    It creates a server that always responds with a simple HTML form.

    To view the page this program displays you should go to http://localhost:5000

*/

#include <iostream>
#include <sstream>
#include <string>
#include <dlib/server.h>
#include <dlib/base64.h>
#include <dlib/md5.h>

using namespace dlib;
using namespace std;

class web_server : public server_http
{
    const std::string on_request ( 
        const incoming_things& incoming,
        outgoing_things& outgoing
    )
    {
        base64 base64_coder;

        ostringstream sout;

        if (incoming.path == "/api/v1/facecompare")
        {
            string img1 = incoming.queries["img1"];
            string img2 = incoming.queries["img2"];
            ostringstream oss1, oss2;
            istringstream iss1, iss2 ;
            iss1.str(img1);
            base64_coder.decode(iss1, oss1);
            string name1 = md5(oss1.str());

            iss2.str(img2);
            base64_coder.decode(iss2, oss2);
            string name2 = md5(oss2.str());
            sout << "md5(img1)=" << name1 << endl;
            sout << "md5(img2)=" << name2 << endl;
        }
        else
        {
            sout << incoming.path << endl;
        }
        sout << "incoming.body: " << incoming.body << endl;
       
        return sout.str();
    }

};

int main()
{
    try
    {
        // create an instance of our web server
        web_server our_web_server;

        // make it listen on port 5000
        our_web_server.set_listening_port(5000);
        // Tell the server to begin accepting connections.
        //our_web_server.start_async();
        our_web_server.start();

    }
    catch (exception& e)
    {
        cout << e.what() << endl;
    }
}


