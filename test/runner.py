import os
from runner_core import pyktx

cur_dir = os.path.dirname(os.path.abspath(__file__))

if __name__ == "__main__":

    input1 = os.path.join(cur_dir, "tga.tga")
    output1 = os.path.join(cur_dir, "output1.ktx2")
    pyktx(f'oidn_app.exe create --format R8G8B8A8_UNORM --zstd 10 --assign-oetf linear --assign-primaries bt709 {input1} {output1}')

    input2 = os.path.join(cur_dir, "한글 띄어쓰기.tga")
    output2 = os.path.join(cur_dir, "output2.ktx2")
    pyktx(f'oidn_app.exe create --format R8G8B8A8_UNORM --zstd 10 --assign-oetf linear --assign-primaries bt709 "{input2}" {output2}')