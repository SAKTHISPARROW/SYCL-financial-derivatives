#include "main_cpu.hpp"
#include "sciplot/Vec.hpp"
#include <random>


// days = No of days to be calculated + 1 is to store initial value for calculation
// samples = No of samples
int GBM(int& days, int& samples, const char* ip_file, std::string op_file, bool is_plot) {

    const double dt = 1/days;          //Scaling factor for drift
    const int size = days*samples;     //Size of buffer

    //Read data
    std::unordered_map<std::string, std::vector<StockData>> stocklists = ReadFile(ip_file);
    std::vector<plt::Figure> plots;
    for(auto& [stockname,data]: stocklists) {
        //Sorting based on date
        std::sort(data.begin(),data.end(),DateSort);
        {
            int t = data.size();
            int b = t -500;
            data = std::vector(data.begin()+b,data.end());
        }

        //Calculate Mean and SD
        auto temp = Mesd(data);
        const double mean =temp.first, stdDev = temp.second;
        const double s0 = data[data.size()-1].close;


        //Buffer for storing paths
        std::vector<double> buf(size);

        for(int index = 0; index < size; index++) {
            std::random_device rd;
            std::mt19937 engine(rd());
            std::normal_distribution<double> z(0.0, 1.0);


            double drift = (mean-(0.5*pow(stdDev,2)))*dt;
            double epx = stdDev * z(engine);
            buf[index] = std::exp( drift + epx );
        }
        for(int index = 0; index < samples; index++) {
            const int i = index*days;
            buf[i] = s0;
            for(int j = 1; j < days; j++) {
                buf[i + j] = buf[ i + j-1] * buf[i+j];
            }
        }



        //Copy to host buffer
        double* res = buf.data();

        if(is_plot) {
            plt::Plot2D plot;
            std::vector<plt::PlotVariant> pl;

            for (int i = 0; i < samples; i++) {
                std::vector<double> sample = std::vector<double>(&res[i*days],&res[i*days+days]);
                plot.drawCurve(plt::linspace(1,days,days),sample).label("");
            }
            plot.xlabel("Days : "+stockname);
            plot.ylabel("Stock Price");
            plot.size(1280, 720);
            pl.emplace_back(plot);
            plt::Figure f = {{pl}};
            f.title(stockname);
            plots.emplace_back(f);
        }

    }

    if(is_plot) {
        // Create canvas to hold figure
        plt::Canvas canvas = {{plots}};
        // Save the plot to a PDF file
        canvas.size(1280,720);
        canvas.save(op_file);
    }

    return 0;
}

void call_error() {
    std::cout<<"Usage"<<std::endl;
    std::cout<<"GBM -i [path to csv file] -o [path to output file] -p [true/false] -samp [number of samples] -days [number of days]"<<std::endl;
    exit(0);
}

#include <string.h>
int main(int argc, char* argv[]) {
    int input_id = 0;
    std::string outpath = "./res.pdf";
    bool is_plot = true;
    int samp = 1000;
    int days = 252;

    for( int i = 1; i < argc; i++) {
        if(strcmp("-i",argv[i]) == 0) {
            if(i+1 < argc) {
                input_id = i+1;
            }
            else call_error();
        }
        if(strcmp("-o",argv[i]) == 0) {
            if(i+1 < argc) {
                outpath = std::string(argv[i+1]);
            }
            else call_error();
        }
        if(strcmp("-p",argv[i]) == 0) {
            if(i+1 < argc) {
                if(strcmp(argv[i+1],"true") == 0) is_plot = true;
                else if(strcmp(argv[i+1],"false") == 0) is_plot = false;
                else {
                    printf("Possible values of -p is true/false but %s provided so falling back to default (true)",argv[i+1]);
                }
            }
            else call_error();
        }
        if(strcmp("-samp",argv[i]) == 0) {
            if(i+1 < argc) {
                samp = atoi(argv[i+1]);
            }
            else call_error();
        }
        if(strcmp("-days",argv[i]) == 0) {
            if(i+1 < argc) {
                days = atoi(argv[i+1]);
            }
            else call_error();
        }
    }

    if(input_id == 0) {
        call_error();
    }

    GBM(days,samp,argv[input_id], outpath,is_plot);
    return 0;
}
