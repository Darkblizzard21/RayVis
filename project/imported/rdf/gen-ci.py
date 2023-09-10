import yaml

ci = {
    'variables':
    {
        'GIT_SUBMODULE_STRATEGY': 'recursive'
    },
    'stages':
    ['build', 'test']
}

def gen_build_windows():
    result = {}

    for stage in {'build', 'test'}:
        for config in {'Debug', 'Release'}:
            name = f'{stage}:windows:{config}'

            entry = {
                'stage': stage,
                'tags': ['windows', 'amd64']
            }

            if stage == 'test':
                entry['needs'] = [f'build:windows:{config}']

            if stage == 'build':
                entry['script'] = [
                    f'cmake.exe -B build -G "Visual Studio 16 2019" -S .',
                    f'cmake.exe --build build --config {config}'
                ]
                entry['artifacts'] = {
                    'paths': [f'.\\build\\bin\\{config}\\*'],
                    'name': f"amdrdf-Win64-{config}-%CI_BUILD_REF_NAME%-%CI_COMMIT_SHORT_SHA%"
                }
            elif stage == 'test':
                entry['script'] = [
                    f'.\\build\\bin\\{config}\\rdf.Test.exe -r junit > report.xml'
                ]
                entry['artifacts'] = {
                    'when': 'always',
                    'reports': {
                        'junit': 'report.xml'
                    }
                }
            
            result[name] = entry

    return result

def gen_build_linux():
    result = {}

    for stage in {'build', 'test'}:
        for config in {'Debug', 'Release'}:
            name = f'{stage}:linux:{config}'

            entry = {
                'stage': stage,
                'tags': ['linux', 'amd64'],
                'image': 'ubuntu:20.04'
            }

            if stage == 'test':
                entry['needs'] = [f'build:linux:{config}']

            if stage == 'build':
                entry['script'] = [
                    f'apt-get update -qq && apt-get install -y wget make g++',
                    f'mkdir cmake', 
                    f'cd cmake', 
                    f'wget -q https://github.com/Kitware/CMake/releases/download/v3.22.0/cmake-3.22.0-Linux-x86_64.sh', 
                    f'chmod +x cmake-3.22.0-Linux-x86_64.sh', 
                    f'./cmake-3.22.0-Linux-x86_64.sh --skip-license', 
                    f'cd ..', 
                    f'./cmake/bin/cmake -B build -G "Unix Makefiles" -DCMAKE_BUILD_TYPE={config} .', 
                    f'./cmake/bin/cmake --build build --parallel', 
                ]
                entry['artifacts'] = {
                    'paths': [
                        './build/bin/*',
                        './build/rdf/libamdrdf.so'
                    ],
                    'name': f"amdrdf-Linux64-{config}-%CI_BUILD_REF_NAME%-%CI_COMMIT_SHORT_SHA%"
                }
            elif stage == 'test':
                entry['script'] = [
                    f'LD_LIBRARY_PATH=./build/rdf ./build/bin/rdf.Test -r junit > report.xml'
                ]
                entry['artifacts'] = {
                    'when': 'always',
                    'reports': {
                        'junit': 'report.xml'
                    }
                }
            
            result[name] = entry

    return result

if __name__ == '__main__':
    ci.update(gen_build_windows())
    ci.update(gen_build_linux())
    yaml.dump(ci, open('.gitlab-ci.yml',  'w'))
