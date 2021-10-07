include Makefile.inc

build: $(BIN)/$(HL_TARGET)/skinDenoise.a $(BIN)/$(HL_TARGET)/skinDetection.a

$(GENERATOR_BIN)/skin_smooth.generator: skin_smooth_generator.cpp $(GENERATOR_DEPS_STATIC)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(filter %.cpp,$^) -o $@ $(LIBHALIDE_LDFLAGS_STATIC)

$(BIN)/%/skinDetection.a: $(GENERATOR_BIN)/skin_smooth.generator
	@mkdir -p $(@D)
	$^ -g skinDetection -o $(@D) target=$*

$(BIN)/%/skinDenoise.a: $(GENERATOR_BIN)/skin_smooth.generator
	@mkdir -p $(@D)
	$^ -g skinDenoise -o $(@D) target=$*

$(BIN)/%/main: main.cpp $(BIN)/%/skinDetection.a $(BIN)/%/skinDenoise.a
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -I$(BIN)/$* -Wall $^ -o $@ $(LDFLAGS) $(IMAGE_IO_FLAGS)

$(BIN)/%/out.jpg: $(BIN)/%/main
	@mkdir -p $(@D)
	$< ./test_images/test1.jpg

test: $(BIN)/$(HL_TARGET)/out.jpg

clean:
	rm -rf $(BIN)