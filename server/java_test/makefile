JUNIT_JAR=junit-platform-console-standalone-1.10.0.jar
JAVA=java
JAVAC=javac
CP=.
Test_FILE = ChatServerTest
all: compile run clean 

run:
	$(JAVA) -jar $(JUNIT_JAR) --class-path $(CP) --scan-class-path

compile:
	$(JAVAC) -cp $(CP):$(JUNIT_JAR) $(Test_FILE).java

clean:
	rm -f *.class
