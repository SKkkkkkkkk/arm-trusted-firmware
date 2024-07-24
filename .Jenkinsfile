import groovy.json.JsonOutput

def getGitBuildTag() {
    def tag = sh(
        script: "git describe --tags `git rev-list --tags --max-count=1` || true",
        returnStdout: true
    ).trim()
    if (tag.contains("fatal")) {
        return ""
    } else {
        return tag
    }
}

pipeline {
    agent any

    environment {
        /* Build config */
        BIUILD_COMPILER = "/sw/tools/arm-gnu-toolchain-13.2.Rel1-x86_64-aarch64-none-elf/bin/aarch64-none-elf-gcc"
        BL33_TARGET = "/sw/tools/tf-a_depend/u-boot.bin"
        NS_BL1U_TARGET = "/sw/tools/tf-a_depend/ns_bl1u.bin"
        /* Build info */
        GIT_REPO_NAME = sh(
            script: "basename \$(git remote get-url origin) .git",
            returnStdout: true
        ).trim()
        GIT_COMMIT_ID = sh(
            script: "git rev-parse --short HEAD",
            returnStdout: true
        ).trim()
        GIT_BUILD_TAG = getGitBuildTag()
        GIT_REPO_OWNER = sh(
            script: "git remote get-url origin | awk -F '/' '{print \$4}'",
            returnStdout: true
        ).trim()
        JFROG_REPO_PATH =   "_0002-a510-g124/sw/repo/" +
                            "${GIT_REPO_OWNER}/" +
                            "${GIT_REPO_NAME}/" +
                            "${BRANCH_NAME}/"
        JFROG_FILE_PROPS =  "repo=${GIT_REPO_NAME};" +
                            "branch=${BRANCH_NAME};" +
                            "commit=${GIT_COMMIT_ID};" +
                            "tag=${GIT_BUILD_TAG};" +
                            "status=ok"
        OUTPUT_PATH = "build/rhea/release"
        UPLOAD_TARGET = "${OUTPUT_PATH}/*.bin"
    }

    stages {
        stage("build project") {
            steps {
                echo "build project"
                script {
                    /* Compile the project and ignore standard output */
                    sh(script:
                        "CROSS_COMPILE=${BIUILD_COMPILER}" +
                        "./build.py" +
                        "--bl33 ${BL33_TARGET}" +
                        "--ns_bl1u ${NS_BL1U_TARGET}" +
                        "> /dev/null"
                    )
                }
            }
        }
        stage("upload binary") {
            steps {
                echo "upload binaries"
                script {
                    if (fileExists(OUTPUT_PATH)) {
                        def files = findFiles(glob: UPLOAD_TARGET)
                        for (file in files) {
                            def path = sh(
                                script: "realpath ${file.path}",
                                returnStdout: true
                            ).trim()

                            fileList.add([
                                pattern: path,
                                target: JFROG_REPO_PATH,
                                props: JFROG_FILE_PROPS,
                                flat: "true"
                            ])
                        }

                        /* Assemble into JSON format */
                        def json = JsonOutput.toJson([files: fileList])
                        println(JsonOutput.prettyPrint(json))
                        /* Upload to artifactory*/
                        def rtServer = Artifactory.server "JFrog"
                        rtServer.upload(spec: json)

                        def buildInfo = Artifactory.newBuildInfo()
                        buildInfo.env.capture = true
                        buildInfo.env.collect()
                        rtServer.publishBuildInfo(buildInfo)
                    } else {
                        error "${OUTPUT_PATH} does not exist."
                    }
                }
            }
        }
    }
    post {
        always {
            echo "clean workspace"
            cleanWs(deleteDirs: true)
        }
    }
}
