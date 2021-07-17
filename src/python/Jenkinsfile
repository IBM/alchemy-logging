#!groovy

//// Config Vars and Functions /////////////////////////////////////////////////

def projectName = "alog-py"
def projectRepo = 'git@github.ibm.com:watson-nlu/alog-py.git'

pipeline {
  agent {
    node 'pickle'
  }

  options {
    buildDiscarder(logRotator(numToKeepStr: "10"))
    timeout(time: 1, unit: 'HOURS')
    timestamps()
    skipDefaultCheckout()
  }

  parameters {

    // Args for publishing the wheel
    booleanParam(defaultValue: false,
      description: "Publish the wheel to artifactory",
      name: "PUBLISH_WHEEL")
  }

  triggers {
    cron( BRANCH_NAME=="main" ? "H H(3-6) * * 1" : "")
  }

  stages {

    // Clean up anything lingering and validate params
    stage('Workspace') {
      steps {
        ansiColor('xterm') {
          deleteDir()
          script {
            def myScm = editableScm()
            myScm['extensions'] = myScm['extensions'] + [
              [$class: 'CloneOption', noTags: false],
            ]
            checkout myScm
          }
        }
      }
    } // Workspace

    // Build the wheel
    stage('Build') {
      steps {
        ansiColor('xterm') {
          script {
            sh 'docker build . --target=wheel -f ./ci/Dockerfile'
          }
        }
      }
    }

    // Run unit tests
    stage('Test') {
      steps {
        ansiColor('xterm') {
          script {
            sh 'docker build . --target=test_wheel -f ./ci/Dockerfile'
            sh 'docker build . --target=test_source -f ./ci/Dockerfile'
          }
        }
      }
    } // Test


    // Publish the wheel
    stage('Publish') {
      when {
        expression { return (env.BRANCH_NAME == "main" || env.PUBLISH_WHEEL == "true") }
      }
      steps {
        ansiColor('xterm') {
          script {
            withCredentials([usernamePassword(credentialsId: 'nlu-functional-id', passwordVariable: 'ARTIFACTORY_API_KEY', usernameVariable: 'ARTIFACTORY_USERNAME')]) {
              currentBranchName = env.BRANCH_NAME
              print "Current Branch: $currentBranchName"
              sh 'docker build . --target=publish -f ./ci/Dockerfile --build-arg ARTIFACTORY_USERNAME=$ARTIFACTORY_USERNAME --build-arg ARTIFACTORY_API_KEY=$ARTIFACTORY_API_KEY'

            }
            echo "build: DONE"
          }
        }
      }
    } // Publish

  } // stages
  post {
    regression {
      script {
        if (env.BRANCH_NAME == "deploy" || env.BRANCH_NAME == "develop" || env.BRANCH_NAME == "main") {
          slackSend channel: 'nlu-alerts', color: 'warning', message: "<!here>: *${env.JOB_NAME}*: <${env.BUILD_URL}console|build ${env.BUILD_DISPLAY_NAME}> failed."
        } else {
          slackSend channel: 'nlu-alerts', color: 'warning', message: "*${env.JOB_NAME}*: <${env.BUILD_URL}console|build ${env.BUILD_DISPLAY_NAME}> failed."
        }
      }
    }
    fixed {
      script {
        if (env.BRANCH_NAME == "deploy" || env.BRANCH_NAME == "develop" || env.BRANCH_NAME == "main") {
          slackSend channel: 'nlu-alerts', color: 'good', message: "<!here>: *${env.JOB_NAME}*: <${env.BUILD_URL}console|build ${env.BUILD_DISPLAY_NAME}> fixed."
        } else {
          slackSend channel: 'nlu-alerts', color: 'good', message: "*${env.JOB_NAME}*: <${env.BUILD_URL}console|build ${env.BUILD_DISPLAY_NAME}> fixed."
        }
      }
    }
    always {
      echo "End of CI pipeline for branch ${env.BRANCH_NAME}."
    }
    cleanup {
      cleanNode();
      deleteDir();
    }
  }
}
