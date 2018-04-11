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

string c(const string& b)
{
    base64 base64_coder;
    ostringstream sout;
    istringstream sin(b);
    base64_coder.decode(sin, sout);
    return sout.str();
}

void write_file(const string& name, const string& content)
{
    ofstream out(name, ios::binary);
    out.write(content.c_str(), content.size());
}

class web_server : public server_http
{
    const std::string on_request ( 
        const incoming_things& incoming,
        outgoing_things& outgoing
    )
    {
        ostringstream sout;

        if (incoming.path == "/api/v1/facecompare")
        {
            string img1 = c(incoming.queries["img1"]);
            string img2 = c(incoming.queries["img2"]);
            string name1 = md5(img1);
            string name2 = md5(img2);
            sout << "md5(img1)=" << name1 << endl;
            sout << "md5(img2)=" << name2 << endl;
            write_file(name1, img1);
            write_file(name2, img2);
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


