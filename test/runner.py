import os
from runner_core import image_denoise, pyktx, test

cur_dir = os.path.dirname(os.path.abspath(__file__))

if __name__ == "__main__":

    # input1 = os.path.join(cur_dir, "tga.tga")
    # output1 = os.path.join(cur_dir, "output1.ktx2")
    # pyktx(f'oidn_app.exe create --format R8G8B8A8_UNORM --zstd 10 --assign-oetf linear --assign-primaries bt709 {input1} {output1}')

    input2 = os.path.join(cur_dir, "한글 띄어쓰기.tga")
    output2 = os.path.join(cur_dir, "한글 띄어쓰기 output.ktx2")
    try:
        pyktx(
            f'oidn_app.exe create --format R8G8B8A8_UNORM --zstd 10 --assign-oetf linear --assign-primaries bt709 "{input2}" "{output2}"'
        )
    except Exception as e:
        print(f"Error processing {input2}: {e}")

    test(
        {
            "Option from python": [
                "arr1",
                "arr2",
                "한글",
            ],
            "boolean value": True,
            "integer value": 42,
            "string value": "Hello, World!",
            "nested dict": {
                "key1": "value1",
                "key2": 123,
                "key3": [1, 2, 3],
            },
            "null value": None,
            "Combined array": [
                "string1",
                123,
                True,
                None,
                {"nested_key": "nested_value"},
            ],
        }
    )

    image_denoise("d:/work/voidn/input.hdr", "d:/work/voidn/hhrrdd.hdr")

    print("Finished")
