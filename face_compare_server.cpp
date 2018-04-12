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
#include <dlib/dir_nav.h>
#include <dlib/cmd_line_parser.h>
#include <dlib/misc_api.h>
#include <dlib/graph_utils.h>

#include "dnn_face_feature.h"

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
            string name1 = dir_ + directory::get_separator() + md5(img1);
            string name2 = dir_ + directory::get_separator() + md5(img2);
            sout << "md5(img1)=" << name1 << endl;
            sout << "md5(img2)=" << name2 << endl;
            if (!file_exists(name1))
            {
                write_file(name1, img1);
            }
            if (!file_exists(name2))
            {
                write_file(name2, img2);
            }

            matrix<float,0,1> m1, m2;
            string msg;
            bool ret;
            ret = feature_util_->extract(name1, m1, msg);
            if (!ret)
            {
                sout << "error: " << msg << endl;
            }
            else
            {
                sout << trans(m1) << endl;
            }
            ret = feature_util_->extract(name2, m2, msg);
            if (!ret)
            {
                sout << "error: " << msg << endl;
            }
            else
            {
                sout << trans(m2) << endl;
                cosine_distance d;
                sout << d(m1, m2) << endl;
            }
           
            sout << msg << endl;
        }
        else
        {
            sout << incoming.path << endl;
        }
        //sout << "incoming.body: " << incoming.body << endl;
       
        return sout.str();
    }

public:
    web_server(string& dir, string& model_dir)
    {
        dir_ = dir;
        if (!file_exists(dir_))
        {
            create_directory(dir_);
        }
        feature_util_ = new face_feature(model_dir);
    }

private:
    string dir_;
    face_feature* feature_util_;
};

int main(int argc, char** argv)
{
    try
    {
        command_line_parser parser;
        parser.add_option("dir","dir for saving image files.",1);
        parser.add_option("model","dir for dnn face model files.",1);
        parser.add_option("h","Display this help message.");

        parser.parse(argc,argv);

        const char* one_time_opts[] = {"dir", "model"};
        parser.check_one_time_options(one_time_opts);

        if (parser.option("h"))
        {
            cout << "Usage: " << argv[0] << " --dir dir_path\n";
            parser.print_options();
            return 0;
        }

        string dir, model_dir;
        if (parser.option("dir"))
        {
            dir = parser.option("dir").argument();
        }
        else
        {
            cout << "Error in command line:\n   You must specify an dir name.\n";
            cout << "\nTry the -h option for more information." << endl;
            return 0;
        }
        if (parser.option("model"))
        {
            model_dir = parser.option("model").argument();
        }
        else
        {
            cout << "Error in command line:\n   You must specify an model dir.\n";
            cout << "\nTry the -h option for more information." << endl;
            return 0;
        }

        // create an instance of our web server
        web_server our_web_server(dir, model_dir);

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


