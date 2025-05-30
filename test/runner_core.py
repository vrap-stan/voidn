import ctypes
import json
import os
import shlex

# DLL 파일의 기본 경로. 이 스크립트 파일과 같은 디렉토리에 있는 "ktxdll.dll"을 가리킵니다.


def find_dll():
    right_next = os.path.join(os.path.dirname(__file__), "ktxdll.dll")
    if os.path.exists(right_next):
        return right_next

    build_test = os.path.join(os.path.dirname(__file__), "../build/vcpp_core.dll")
    if os.path.exists(build_test):
        return build_test

    # warn
    print(
        "KTX DLL 파일을 찾을 수 없습니다. 현재 디렉토리에서 'ktxdll.dll' 또는 '../build/vcpp_core.dll'을 확인하세요."
    )
    return None


DLL_PATH = find_dll()
CORE_DLL = None  # ctypes로 로드된 DLL 객체를 저장할 변수
CORE_KTX_FUNC = None  # DLL 내의 vcpp_ktx 함수 포인터를 저장할 변수
CORE_FBX_FUNC = None  # DLL 내의 vcpp_fbx 함수 포인터를 저장할 변수
CORE_OIDN_FUNC = None  # DLL 내의 vcpp_image 함수 포인터를 저장할 변수
CORE_TEST_FUNC = None  # DLL 내의 vcpp_test 함수 포인터를 저장할 변수


def set_dll_path(dll_path: str, *, init=True):
    """
    KTX DLL 파일의 경로를 설정합니다.

    Args:
        dll_path (str): 사용할 KTX DLL 파일의 전체 경로.
        init (bool, optional): 경로 설정 후 즉시 DLL을 초기화할지 여부. 기본값은 True.
    """
    global DLL_PATH
    DLL_PATH = dll_path

    if init:
        init_dll()


def strip_quotes(s):
    if s.startswith('"') and s.endswith('"'):
        retval = s[1:-1]
        return strip_quotes(retval)
    return s


def parse_cli_string(command_string):
    """
    명령줄 문자열을 셸과 유사하게 따옴표를 존중하며 인자 리스트로 파싱합니다.

    Args:
        command_string (str): 파싱할 명령줄 문자열.

    Returns:
        list: 각 문자열이 인자인 문자열 리스트.
              파싱에 실패하거나 입력이 비어 있으면 빈 리스트를 반환합니다.
    """
    if not command_string:
        return []
    try:
        # shlex.split은 셸처럼 따옴표와 공백을 처리합니다.
        # posix=False는 Windows 스타일 경로(\)를 이스케이프 문자로 취급하지 않도록 합니다.
        args = shlex.split(command_string, posix=False)
        return [strip_quotes(arg) for arg in args]
    except ValueError as e:
        print(f"명령줄 문자열 파싱 오류: {e}")
        print("이 오류는 주로 따옴표가 맞지 않을 때 발생합니다.")
        return (
            []
        )  # 또는 더 엄격한 오류 처리를 원한다면 예외를 다시 발생시킬 수 있습니다.


def init_dll():
    """
    설정된 DLL_PATH를 사용하여 KTX DLL을 로드하고 vpp_ktx 함수를 준비합니다.
    이 함수는 스크립트 시작 시 또는 set_dll_path 호출 시 실행됩니다.
    """
    global DLL_PATH, CORE_DLL, CORE_KTX_FUNC, CORE_FBX_FUNC, CORE_OIDN_FUNC, CORE_TEST_FUNC

    if DLL_PATH is None or DLL_PATH == "" or not os.path.exists(DLL_PATH):
        print(f"DLL 경로를 설정해주세요. 현재 설정된 경로: {DLL_PATH}")
        exit(1)

    try:
        CORE_DLL = ctypes.CDLL(DLL_PATH)
    except OSError as e:
        print(f"DLL '{DLL_PATH}' 로딩 오류: {e}")
        print(
            "DLL 파일이 존재하고 시스템의 PATH 또는 스크립트 디렉토리에 있는지 확인하세요."
        )
        exit(1)

    # 2. vpp_ktx 함수의 시그니처를 정의합니다.
    # C/C++에서의 함수 선언: int vpp_ktx(int argc, char* argv[])
    try:
        CORE_KTX_FUNC = CORE_DLL.vcpp_ktx
        CORE_KTX_FUNC.argtypes = [
            ctypes.c_int,
            ctypes.POINTER(ctypes.c_char_p),
            ctypes.c_char_p,
        ]
        CORE_KTX_FUNC.restype = ctypes.c_int
    except AttributeError:
        print(f"오류: DLL '{DLL_PATH}'에서 'vpp_ktx' 함수를 찾을 수 없습니다.")
        print(
            '함수가 올바르게 익스포트되었는지 확인하세요 (예: __declspec(dllexport) 및 extern "C" 사용).'
        )
        exit(1)

    try:
        CORE_FBX_FUNC = CORE_DLL.vcpp_fbx
        CORE_FBX_FUNC.argtypes = [
            ctypes.c_int,
            ctypes.POINTER(ctypes.c_char_p),
            ctypes.c_char_p,
        ]
        CORE_FBX_FUNC.restype = ctypes.c_int
    except AttributeError:
        print(f"오류: DLL '{DLL_PATH}'에서 'vcpp_fbx' 함수를 찾을 수 없습니다.")
        print(
            '함수가 올바르게 익스포트되었는지 확인하세요 (예: __declspec(dllexport) 및 extern "C" 사용).'
        )
        exit(1)

    try:
        CORE_OIDN_FUNC = CORE_DLL.vcpp_image
        CORE_OIDN_FUNC.argtypes = [
            ctypes.c_int,
            ctypes.POINTER(ctypes.c_char_p),
            ctypes.c_char_p,
        ]
        CORE_OIDN_FUNC.restype = ctypes.c_int
    except AttributeError:
        print(f"오류: DLL '{DLL_PATH}'에서 'oidn' 함수를 찾을 수 없습니다.")
        print(
            '함수가 올바르게 익스포트되었는지 확인하세요 (예: __declspec(dllexport) 및 extern "C" 사용).'
        )
        exit(1)

    try:
        CORE_TEST_FUNC = CORE_DLL.vcpp_test
        CORE_TEST_FUNC.argtypes = [
            ctypes.c_int,
            ctypes.POINTER(ctypes.c_char_p),
            ctypes.c_char_p,
        ]
        CORE_TEST_FUNC.restype = ctypes.c_int
    except AttributeError:
        print(f"오류: DLL '{DLL_PATH}'에서 'vcpp_test' 함수를 찾을 수 없습니다.")
        print(
            '함수가 올바르게 익스포트되었는지 확인하세요 (예: __declspec(dllexport) 및 extern "C" 사용).'
        )
        exit(1)


# 스크립트가 임포트될 때 DLL을 초기화합니다.
init_dll()


def pyktx(cmd: str, options: dict = {}):
    """
    단일 명령줄 문자열을 받아 파싱한 후 ktx_main을 호출합니다.

    Args:
        cmd (str): 실행할 ktx 명령줄 문자열 (예: "ktx create --format R8G8B8A8_UNORM input.tga output.ktx2").

    Returns:
        int: vpp_ktx 함수에서 반환된 정수형 반환 코드.
    """
    return pyktx_list(parse_cli_string(cmd), options=options)


def pyktx_list(cmd_list: list[str], options: dict = {}):
    """
    이미 파싱된 명령줄 인자 리스트를 받아 DLL의 vpp_ktx 함수를 호출합니다.

    Args:
        cmd_list (list[str]): 명령줄 인자를 나타내는 문자열 리스트.
                              예: ["ktx.exe", "create", "--format", "R8G8B8A8_UNORM", "input.tga", "output.ktx2"]
                              첫 번째 인자는 일반적으로 프로그램 이름 자체입니다.
    Returns:
        int: vpp_ktx 함수에서 반환된 정수형 반환 코드.
    """
    global CORE_KTX_FUNC
    if CORE_KTX_FUNC is None:
        print(
            "오류: KTX DLL 또는 vpp_ktx 함수가 초기화되지 않았습니다. init_dll()을 먼저 호출하세요."
        )
        return -1  # 또는 적절한 오류 코드

    argc = len(cmd_list)

    # Python 문자열을 UTF-8로 인코딩된 바이트 문자열로 변환합니다.
    argv_bytes = [arg.encode("utf-8") for arg in cmd_list]

    # C 스타일 문자열(char*)의 ctypes 배열을 만듭니다.
    # ctypes.c_char_p는 char에 대한 포인터입니다 (사실상 char*).
    argv_c = (ctypes.c_char_p * argc)()
    for i, arg_byte_str in enumerate(argv_bytes):
        argv_c[i] = arg_byte_str

    # print(f"argc={argc}, argv={[arg for arg in cmd_list]}로 vpp_ktx 호출 중")

    # 함수를 호출합니다.
    return_code = CORE_KTX_FUNC(argc, argv_c)

    # print(f"vpp_ktx 반환 값: {return_code}")
    return return_code


def test(options: dict):
    if CORE_TEST_FUNC is None:
        print(
            "오류: KTX DLL 또는 vpp_test 함수가 초기화되지 않았습니다. init_dll()을 먼저 호출하세요."
        )
        return -1

    cmd_list = ["test arg1", "test arg2"]

    argc = len(cmd_list)
    argv_bytes = [arg.encode("utf-8") for arg in cmd_list]

    argv_c = (ctypes.c_char_p * argc)()
    for i, arg_byte_str in enumerate(argv_bytes):
        argv_c[i] = arg_byte_str

    options_str = json.dumps(options).encode("utf-8")

    return_code = CORE_TEST_FUNC(argc, argv_c, options_str)
    return return_code
