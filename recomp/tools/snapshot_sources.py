"""Bundle committed local sources for offline restore; no game data or network."""
from pathlib import Path
import argparse,hashlib,json,subprocess
ROOT=Path(__file__).resolve().parents[1]
def main():
    p=argparse.ArgumentParser();p.add_argument('--git',type=Path,required=True)
    p.add_argument('--foundation',type=Path,default=ROOT.parent/'PS5_Sims2Independent')
    a=p.parse_args();git=str(a.git.resolve())
    def run(repo,*args):
        return subprocess.check_output([git,'-C',str(repo),*args],text=True,stderr=subprocess.STDOUT).strip()
    repos={'project':ROOT,'tools-sdk':ROOT/'sdk','runtime-sdk':ROOT/'runtime-sdk','ps5-foundation':a.foundation.resolve()}
    revisions={}
    for name,repo in repos.items():
        if run(repo,'status','--porcelain'):raise SystemExit('Commit/preserve local source changes first: '+name)
        revisions[name]=run(repo,'rev-parse','HEAD')
    out=ROOT/'out/source-bundles'/revisions['project'][:12];out.mkdir(parents=True,exist_ok=False)
    records={}
    for name,repo in repos.items():
        bundle=out/(name+'.bundle')
        # HEAD explicitly includes a detached submodule integration revision.
        run(repo,'bundle','create',str(bundle),'--all','HEAD')
        verify=run(repo,'bundle','verify',str(bundle))
        (out/(name+'-verify.log')).write_text(verify+'\n')
        with bundle.open('rb') as stream:digest=hashlib.file_digest(stream,'sha256').hexdigest()
        records[name]={'revision':revisions[name],'file':bundle.name,'bytes':bundle.stat().st_size,'sha256':digest}
    (out/'manifest.json').write_text(json.dumps({'schema':1,'scope':'Committed source only; no owner game data or generated game code','components':records},indent=2)+'\n')
    print(json.dumps({'status':'PASS_SOURCE_BUNDLES','directory':str(out),'components':len(records)}))
if __name__=='__main__':main()
