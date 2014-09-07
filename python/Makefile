include ../mf/common

ifeq ($(TARGET_OS),windows)
	ifndef CAN_LINK_PYTHON
		ifeq (,$(wildcard $(dir $(PYTHON))/DLLs/python3.dll))
$(error Needs absolute path to PYTHON. Files missing in the default Windows Python distribution need to be created. (make CAN_LINK_PYTHON=1 to override))
		else
			PYTHON_PATH = $(dir $(PYTHON))
			ALL_EXTRA_DEPS = python34.def python34.lib
		endif
	endif
endif

all: $(ALL_EXTRA_DEPS)
	$(PYTHON) setup.py build $(PYTHON_SETUP_BUILD_FLAGS)

ifeq ($(TARGET_OS),windows)
python34.def:
	sh mksyms.sh $(PYTHON_PATH)/DLLs/python3.dll > "$@"

python34.lib: python34.def
	dlltool --dllname python34.dll --input-def "$<" --output-lib "$@"
endif

clean:
	rm -rf build/
	rm -f python34.def python34.lib

