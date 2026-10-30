"""Compare the same idle render workload against an immutable Git reference.
Software measurement only; no iOS/AUM/plug-in CPU or realtime certification.
"""
from pathlib import Path
import argparse,subprocess,zipfile,io,json,statistics,platform,hashlib
CPP = "#include \"Shared/XoneHostEngine.hpp\"\n#include <array>\n#include <algorithm>\n#include <chrono>\n#include <cassert>\n#include <iostream>\nusing namespace xone::host;\nint main(){\n HostEngine e;e.enableNativeMutePage(true);e.observeTransport(false);\n using Clock=std::chrono::steady_clock;\n constexpr unsigned Warm=256,Blocks=4096;\n std::array<double,Blocks> times{};unsigned leds=0;\n for(unsigned i=0;i<Warm+Blocks;++i){\n  const auto start=Clock::now();e.beginRenderBuffer();e.beginSourceRetryPass();e.beginCueLoadRetryPass();\n  e.onMusicalGrid(GridBatch{});e.advance(i*128*1000/48000,i*128);e.prepareMuteLeds(i*128*1000/48000);\n  unsigned cursor=0;for(unsigned n=0;n<136;++n){auto p=e.nextMuteLedPacket(cursor);if(!p.valid)break;e.ackMuteLedPacket(p,true);++leds;}\n  e.endRenderBuffer();\n  if(i>=Warm)times[i-Warm]=std::chrono::duration<double,std::micro>(Clock::now()-start).count();\n }\n assert(e.overflows()==0&&leds==100);\n double total=0;for(double x:times)total+=x;std::sort(times.begin(),times.end());\n std::cout<<\"{\\\"mean_us\\\":\"<<total/Blocks<<\",\\\"p99_us\\\":\"<<times[Blocks*99/100]<<\",\\\"max_us\\\":\"<<times.back()<<\",\\\"document_bytes\\\":\"<<sizeof(DocumentImage)<<\",\\\"engine_bytes\\\":\"<<sizeof(HostEngine)<<\"}\\n\";\n}\n"
def main():
 parser=argparse.ArgumentParser(description=__doc__)
 parser.add_argument('--compiler',required=True)
 parser.add_argument('--reference',default='aa6195c')
 parser.add_argument('--optimization',choices=['O0','O1','O2','O3'],default='O2')
 parser.add_argument('--output',default='evidence/reset-cpu/benchmark')
 args=parser.parse_args()
 repo=Path(__file__).resolve().parents[1]
 out=(repo/args.output).resolve()
 if not out.is_relative_to(repo):raise SystemExit('Output must stay inside this checkout')
 out.mkdir(parents=True,exist_ok=True)
 sha=subprocess.check_output(['git','rev-parse',args.reference],cwd=repo,text=True).strip()
 raw=subprocess.check_output(['git','archive','--format=zip',sha,'Core','CoreV3','Shared'],cwd=repo)
 ref=out/('reference-'+sha[:12]);ref.mkdir(exist_ok=True)
 with zipfile.ZipFile(io.BytesIO(raw)) as archive:
  for item in archive.infolist():
   dest=(ref/item.filename).resolve()
   if not dest.is_relative_to(ref.resolve()):raise SystemExit('Invalid archive path')
  archive.extractall(ref)
 source=out/'idle_comparison.cpp';source.write_text(CPP,encoding='utf-8')
 executables={}
 for name,include in [('reference',ref),('working',repo)]:
  exe=out/(name+('.exe' if platform.system()=='Windows' else ''))
  cmd=[args.compiler,'-std=c++17','-'+args.optimization,'-UNDEBUG','-Wall','-Wextra','-Wpedantic','-Werror','-pthread']
  if platform.system()=='Windows':cmd+=['-static','-Wl,--stack,16777216']
  cmd+=['-I',str(include),str(source),'-o',str(exe)]
  with (out/(name+'.compile.log')).open('w',encoding='utf-8') as log:
   subprocess.run(cmd,cwd=repo,stdout=log,stderr=subprocess.STDOUT,check=True)
  executables[name]=exe
 digest=hashlib.sha256()
 for folder in ['Core','CoreV3','Shared']:
  for p in sorted((repo/folder).glob('*.hpp')):
   digest.update(str(p.relative_to(repo)).replace('\\','/').encode());digest.update(p.read_bytes())
 samples={'reference':[],'working':[]}
 for i in range(5):
  for name in (['reference','working'] if i%2==0 else ['working','reference']):
   samples[name].append(json.loads(subprocess.check_output([str(executables[name])],text=True)))
 result={'reference_sha':sha,'working_head':subprocess.check_output(['git','rev-parse','HEAD'],cwd=repo,text=True).strip(),
         'working_changes_included':True,'working_headers_sha256':digest.hexdigest(),'platform':platform.platform(),
         'compiler':subprocess.check_output([args.compiler,'--version'],text=True).splitlines()[0],
         'flags':'C++17 '+args.optimization+' assertions and warnings fatal','warmup':256,'buffers_per_run':4096,'frames':128,
         'sample_rate':48000,'samples':samples,'scope':'isolated native idle workload; not iOS/AUM CPU or real-time guarantee'}
 for name in samples:
  result[name+'_median_mean_us']=statistics.median(s['mean_us'] for s in samples[name])
 result['idle_time_ratio_reference_over_working']=result['reference_median_mean_us']/result['working_median_mean_us']
 (out/'RESULTATS.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
 print(json.dumps(result,indent=2))
if __name__=='__main__':main()
