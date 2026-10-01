// Offline coherent spectral matrix. High-rate references use the SAME original
// model at 8x and 16x base rate (its own 8x oversampling remains enabled).
#include "core/Effects.h"
#include "dsp/Amplifiers.h"
#include <algorithm>
#include <complex>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numbers>
using namespace guitarrack;
namespace {
constexpr unsigned N=2048;constexpr double rate=48000,tau=2*std::numbers::pi;
void fft(std::vector<std::complex<double>>& x){const auto size=x.size();for(std::size_t i=1,j=0;i<size;++i){auto bit=size>>1;for(;j&bit;bit>>=1)j^=bit;j^=bit;if(i<j)std::swap(x[i],x[j]);}for(std::size_t n=2;n<=size;n*=2){auto w=std::polar(1.,-tau/n);for(std::size_t at=0;at<size;at+=n){std::complex<double> p=1;for(std::size_t j=0;j<n/2;++j){auto a=x[at+j],b=p*x[at+j+n/2];x[at+j]=a+b;x[at+j+n/2]=a-b;p*=w;}}}}
std::vector<std::complex<double>> spectrum(const EffectDescriptor& d,unsigned factor,unsigned bin,double input,double drive){
 ParameterBank bank;for(const auto& p:d.parameters){float v=p.defaultValue;if(p.key=="gain_db"||p.key=="drive_db")v=float(drive);if(p.key=="sag_percent")v=0;if(p.key=="output_db")v=-12;bank.setValue(0,p.bankIndex,p.normalize(v));}
 auto processor=d.factory(bank,0);if(!processor->prepare(rate*factor,512))throw std::runtime_error("Reference preparation failed");processor->reset();
 const unsigned length=N*factor,warm=unsigned(rate*factor*.12);std::vector<std::complex<double>> result(length);std::array<float,512> l{},r{};
 for(unsigned at=0;at<warm+length;at+=512){const unsigned count=std::min(512u,warm+length-at);for(unsigned i=0;i<count;++i)l[i]=r[i]=float(input*std::sin(tau*bin*(at+i)/(N*factor)));processor->process(l.data(),r.data(),int(count));for(unsigned i=0;i<count;++i)if(at+i>=warm)result[at+i-warm]=l[i];}
 fft(result);for(auto& x:result)x/=length;return result;
}
double nonharmonic(const std::vector<std::complex<double>>& x,unsigned fundamental){double energy=0;for(unsigned bin=4;bin<N/2;++bin){bool intended=false;for(unsigned h=1;h*fundamental<N/2;++h)if(std::abs(int(bin)-int(h*fundamental))<=3){intended=true;break;}if(!intended)energy+=std::norm(x[bin]);}return energy;}
double db(double energy,double fundamental){return 10*std::log10(std::max(1e-30,energy)/std::max(1e-30,fundamental));}
}
int main(int argc,char** argv){if(argc!=2){std::cerr<<"Usage: guitarrack_alias_audit output.csv\n";return 2;}try{
 std::ofstream csv(argv[1]);csv<<"type,base_rate,hz,input_peak,drive_db,alias_to_fundamental_db,ref8_nonharmonic_db,ref16_nonharmonic_db,ref8_to_ref16_magnitude_delta_db\n"<<std::setprecision(9);
 auto descriptors=amplifierDescriptors();auto drives=driveDescriptors();descriptors.insert(descriptors.end(),drives.begin(),drives.end());
 for(const auto& d:descriptors){const auto control=std::find_if(d.parameters.begin(),d.parameters.end(),[](const auto& p){return p.key=="gain_db"||p.key=="drive_db";});
  for(double hz:{300.,3000.,9000.})for(double input:{.03,.3})for(double drive:{double(control->defaultValue),double(control->maximum)}){
   const unsigned bin=unsigned(std::round(hz*N/rate));auto base=spectrum(d,1,bin,input,drive),ref8=spectrum(d,8,bin,input,drive),ref16=spectrum(d,16,bin,input,drive);double error=0,total=0;
   for(unsigned i=0;i<N/2;++i){error+=std::pow(std::abs(ref8[i])-std::abs(ref16[i]),2);total+=std::norm(ref16[i]);}
   csv<<d.typeId<<','<<rate<<','<<double(bin)*rate/N<<','<<input<<','<<drive<<','<<db(nonharmonic(base,bin),std::norm(base[bin]))<<','<<db(nonharmonic(ref8,bin),std::norm(ref8[bin]))<<','<<db(nonharmonic(ref16,bin),std::norm(ref16[bin]))<<','<<db(error,total)<<'\n';csv.flush();
  }
 }return csv?0:1;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
