#include <dlib/dnn.h>
#include <dlib/clustering.h>
#include <dlib/string.h>
#include <dlib/image_io.h>
#include <dlib/image_processing/frontal_face_detector.h>

using namespace dlib;
using namespace std;

// ----------------------------------------------------------------------------------------

// The next bit of code defines a ResNet network.  It's basically copied
// and pasted from the dnn_imagenet_ex.cpp example, except we replaced the loss
// layer with loss_metric and made the network somewhat smaller.  Go read the introductory
// dlib DNN examples to learn what all this stuff means.
//
// Also, the dnn_metric_learning_on_images_ex.cpp example shows how to train this network.
// The dlib_face_recognition_resnet_model_v1 model used by this example was trained using
// essentially the code shown in dnn_metric_learning_on_images_ex.cpp except the
// mini-batches were made larger (35x15 instead of 5x5), the iterations without progress
// was set to 10000, and the training dataset consisted of about 3 million images instead of
// 55.  Also, the input layer was locked to images of size 150.
template <template <int,template<typename>class,int,typename> class block, int N, template<typename>class BN, typename SUBNET>
using residual = add_prev1<block<N,BN,1,tag1<SUBNET>>>;

template <template <int,template<typename>class,int,typename> class block, int N, template<typename>class BN, typename SUBNET>
using residual_down = add_prev2<avg_pool<2,2,2,2,skip1<tag2<block<N,BN,2,tag1<SUBNET>>>>>>;

template <int N, template <typename> class BN, int stride, typename SUBNET> 
using block  = BN<con<N,3,3,1,1,relu<BN<con<N,3,3,stride,stride,SUBNET>>>>>;

template <int N, typename SUBNET> using ares      = relu<residual<block,N,affine,SUBNET>>;
template <int N, typename SUBNET> using ares_down = relu<residual_down<block,N,affine,SUBNET>>;

template <typename SUBNET> using alevel0 = ares_down<256,SUBNET>;
template <typename SUBNET> using alevel1 = ares<256,ares<256,ares_down<256,SUBNET>>>;
template <typename SUBNET> using alevel2 = ares<128,ares<128,ares_down<128,SUBNET>>>;
template <typename SUBNET> using alevel3 = ares<64,ares<64,ares<64,ares_down<64,SUBNET>>>>;
template <typename SUBNET> using alevel4 = ares<32,ares<32,ares<32,SUBNET>>>;

using anet_type = loss_metric<fc_no_bias<128,avg_pool_everything<
                            alevel0<
                            alevel1<
                            alevel2<
                            alevel3<
                            alevel4<
                            max_pool<3,3,2,2,relu<affine<con<32,7,7,2,2,
                            input_rgb_image_sized<150>
                            >>>>>>>>>>>>;

// ----------------------------------------------------------------------------------------

std::vector<matrix<rgb_pixel>> jitter_image(
    const matrix<rgb_pixel>& img
);

class face_feature
{
public:
    face_feature(const string& model_dir)
    {
        detector = get_frontal_face_detector();
        deserialize(model_dir + directory::get_separator() +
                "shape_predictor_5_face_landmarks.dat") >> sp;
        deserialize(model_dir + directory::get_separator() +
                "dlib_face_recognition_resnet_model_v1.dat") >> net;
    }

    int extract(const string& file_name, matrix<float,0,1>& face_descriptor, string& msg);
private:
    frontal_face_detector detector;
    shape_predictor sp;
    anet_type net;
};

// ----------------------------------------------------------------------------------------

int face_feature::extract(const string& file_name,
        matrix<float,0,1>& face_descriptor, string& msg)
{
    matrix<rgb_pixel> img;
    load_image(img, file_name);

    ostringstream oss;

    std::vector<matrix<rgb_pixel>> faces;
    for (auto face : detector(img))
    {
        auto shape = sp(img, face);
        matrix<rgb_pixel> face_chip;
        extract_image_chip(img, get_face_chip_details(shape,150,0.25), face_chip);
        faces.push_back(move(face_chip));
    }

    if (faces.size() != 1)
    {
        oss << "should only contain one face in image, " << faces.size() << " faces found!";
        msg = oss.str();
        return faces.size();
    }

    // This call asks the DNN to convert each face image in faces into a 128D vector.
    // In this 128D vector space, images from the same person will be close to each other
    // but vectors from different people will be far apart.  So we can use these vectors to
    // identify if a pair of images are from the same person or from different people.  
    std::vector<matrix<float,0,1>> face_descriptors = net(faces);
   
    // It should also be noted that face recognition accuracy can be improved if jittering
    // is used when creating face descriptors.  In particular, to get 99.38% on the LFW
    // benchmark you need to use the jitter_image() routine to compute the descriptors,
    // like so:
    face_descriptor = mean(mat(net(jitter_image(faces[0]))));
    //cout << "jittered face descriptor for one face: " << trans(face_descriptor) << endl;
    // If you use the model without jittering, as we did when clustering the bald guys, it
    // gets an accuracy of 99.13% on the LFW benchmark.  So jittering makes the whole
    // procedure a little more accurate but makes face descriptor calculation slower.
    return 1;
}

// ----------------------------------------------------------------------------------------

std::vector<matrix<rgb_pixel>> jitter_image(
    const matrix<rgb_pixel>& img
)
{
    // All this function does is make 100 copies of img, all slightly jittered by being
    // zoomed, rotated, and translated a little bit differently. They are also randomly
    // mirrored left to right.
    thread_local dlib::rand rnd;

    std::vector<matrix<rgb_pixel>> crops; 
    for (int i = 0; i < 100; ++i)
        crops.push_back(jitter_image(img,rnd));

    return crops;
}

// ----------------------------------------------------------------------------------------

