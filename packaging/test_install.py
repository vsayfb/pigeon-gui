import hashlib,io,os,subprocess,tarfile,tempfile
from pathlib import Path
with tempfile.TemporaryDirectory(prefix='pigeon-gui-install-') as tmp:
    t=Path(tmp); release=t/'release'; (release/'releases/0.1.0').mkdir(parents=True)
    package='pigeon-gui-linux-x86_64-glibc'; archive=io.BytesIO()
    with tarfile.open(fileobj=archive,mode='w:gz') as tar:
        for name,data in {'VERSION':b'0.1.0','PLATFORM':b'x86_64-glibc','COMPONENT':b'gui','bin/pigeon-gui':Path('packaging/pigeon-gui.sh').read_bytes(),'bin/pigeon-gui.real':b'#!/bin/sh\nexit 0\n','share/uninstall.sh':Path('packaging/uninstall.sh').read_bytes(),'share/pigeon/pigeon.png':b'icon fixture'}.items():
            info=tarfile.TarInfo(name); info.size=len(data); info.mode=0o755 if name.startswith('bin/') else 0o644
            tar.addfile(info,io.BytesIO(data))
    data=archive.getvalue(); sha=hashlib.sha256(data).hexdigest()
    (release/'latest.txt').write_text('0.1.0\n')
    (release/f'releases/0.1.0/{package}.tar.gz.part000').write_bytes(data)
    (release/f'releases/0.1.0/{package}.manifest').write_text(f'{sha} {package}.tar.gz\n{sha} {package}.tar.gz.part000\n')
    env=os.environ|{'PIGEON_INSTALL_ROOT':str(t/'data with spaces'),'PIGEON_BIN_DIR':str(t/'bin with spaces'),'XDG_DATA_HOME':str(t/'desktop data')}
    for _ in range(2):
        subprocess.run(['sh','packaging/install.sh','--from',str(release)],env=env,check=True,stdout=subprocess.DEVNULL)
    desktop=t/'desktop data/applications/pigeon-gui.desktop'
    text=desktop.read_text(); assert 'Name=Pigeon\n' in text and f'Exec="{t}/bin with spaces/pigeon-gui"' in text
    assert (t/'data with spaces/desktop/pigeon.png').read_bytes()==b'icon fixture'
    original = text
    desktop.write_text('[Desktop Entry]\nName=Another app\n')
    result=subprocess.run(['sh','packaging/install.sh','--from',str(release)],env=env,capture_output=True)
    assert result.returncode and 'Another app' in desktop.read_text()
    desktop.write_text(original)
    history = t/'data with spaces/pigeon.db'
    history.write_text('history')
    subprocess.run([str(t/'bin with spaces/pigeon-gui'), '--uninstall'], check=True)
    assert history.read_text() == 'history' and not desktop.exists()
    assert not (t/'bin with spaces/pigeon-gui').is_symlink()
print('GUI installer checks passed: desktop/icon registration, spaced paths, upgrades, unrelated entry preservation')
