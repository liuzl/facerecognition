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
#include <dlib/logger.h>
#include "dnn_face_feature.h"

using namespace dlib;
using namespace std;

logger xlog("facecompare");

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
	void on_connect (
            std::istream& in,
            std::ostream& out,
            const std::string& foreign_ip,
            const std::string& local_ip,
            unsigned short foreign_port,
            unsigned short local_port,
            uint64
    )
	{
        ostringstream sout;
        outgoing_things outgoing;
        outgoing.headers["content-type"] = "application/json;charset=utf-8";
        try
        {
            incoming_things incoming(foreign_ip, local_ip, foreign_port, local_port);
            parse_http_request(in, incoming, get_max_content_length());
            read_body(in, incoming);
            const std::string& result = on_request(incoming, outgoing);
            write_http_response(out, outgoing, result);
        }
        catch (http_parse_error& e)
        {
            xlog << LERROR << "Error processing request from: " << foreign_ip << " - " << e.what();
            //write_http_response(out, e);
            sout << "{\"message\":\"" << e.what()
                <<"\",\"data\":null,\"extra\":null,\"code\":\"IMAGE_INVALID\"}";
            write_http_response(out, outgoing, sout.str());
        }
        catch (std::exception& e)
        {
            xlog << LERROR << "Error processing request from: " << foreign_ip << " - " << e.what();
            //write_http_response(out, e);
            sout << "{\"message\":\"" << e.what()
                <<"\",\"data\":null,\"extra\":null,\"code\":\"IMAGE_INVALID\"}";
            write_http_response(out, outgoing, sout.str());
        }
	}

    const std::string on_request ( 
        const incoming_things& incoming,
        outgoing_things& outgoing
    )
    {
        xlog << LINFO << incoming.foreign_ip << "\t"
            << incoming.request_type << "\t"
            << incoming.path;
        if (incoming.path.find("/api/v1/facecompare") != 0)
        {
            xlog << LERROR << "invalid request";
            return "{\"code\":\"INVALID_REQUEST\"}";
        }

        string img1 = c(incoming.queries["img1"]);
        string img2 = c(incoming.queries["img2"]);
        string name1 = dir_ + directory::get_separator() + md5(img1);
        string name2 = dir_ + directory::get_separator() + md5(img2);
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
        int ret;
        ostringstream sout;
        ret = feature_util_->extract(name1, m1, msg);
        if (ret != 1)
        {
            sout << "{\"message\":\"" << msg
                <<"\",\"data\":null,\"extra\":null,\"code\":\"IMAGE_FACE_COUNT\"}";
            return sout.str();
        }
        ret = feature_util_->extract(name2, m2, msg);
        if (ret != 1)
        {
            sout << "{\"message\":\"" << msg
                <<"\",\"data\":null,\"extra\":null,\"code\":\"IMAGE_FACE_COUNT\"}";
            return sout.str();
        }

        cosine_distance d;
        double s = (1 - d(m1, m2)) * 100;
        sout << "{\"message\":\"OK\",\"data\":{\"similarity\":" << s
            << "},\"extra\":null,\"code\":\"SUCCESS\"}";
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
    xlog.set_level(LALL);
    try
    {
        command_line_parser parser;
        parser.add_option("dir", "dir for saving image files.", 1);
        parser.add_option("model", "dir for dnn face model files.", 1);
        parser.add_option("host", "listen ip.", 1);
        parser.add_option("port", "listen port.", 1);
        parser.add_option("h", "Display this help message.");

        parser.parse(argc,argv);
        const char* one_time_opts[] = {"dir", "model"};
        parser.check_one_time_options(one_time_opts);
        parser.check_option_arg_range("port", 80, 65535);
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
            cout << "Error in cmd line:\n   You must specify a dir.\n";
            cout << "\nTry the -h option for more information." << endl;
            return 0;
        }
        if (parser.option("model"))
        {
            model_dir = parser.option("model").argument();
        }
        else
        {
            cout << "Error in cmd line:\n   You must specify a model dir.\n";
            cout << "\nTry the -h option for more information." << endl;
            return 0;
        }
        int port = get_option(parser,"port", 5000);
        string ip = get_option(parser,"host", "127.0.0.1");

        web_server our_web_server(dir, model_dir);
        our_web_server.set_listening_port(port);
        our_web_server.set_listening_ip(ip);
        xlog << LINFO << "server listen on "
            << our_web_server.get_listening_ip() << ":" << port;
        our_web_server.start();
    }
    catch (exception& e)
    {
        cout << e.what() << endl;
    }
}

