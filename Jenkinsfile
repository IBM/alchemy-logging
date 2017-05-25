#!groovy

//// Config Vars and Functions /////////////////////////////////////////////////

def projectRepo = 'https://github.ibm.com/watson-discovery/alog.git'
def dockerRepo = "haproxy-v2.wdc.dev.ibmcsf.net"
def baseImageVersion = "2.0.0"
def depsImageName = "disco-ds/deps"
def depsImageVersion = "2.0.0"
def dockerArgs = "--volume=/var/run/docker.sock:/var/run/docker.sock:rw"
def nodeLabel = "pickle"
def versionBumpChoices = '''NONE
Major
Minor
Sub'''
def versionBumpDescription = '''<p>Choose what type of version bump this is. The version format is:</p>

<p>Major.minor.sub</p>

<p>The general guidelines for choosing the version bump type are:</p>
<bl>
<li>Major => Significant service-altering change. All API breaking changes.</li>
<li>Sub => Small bugfix to previous version</li>
<li>Minor => All other new versions (i.e. new functionality)</li>
</bl>'''

// NOTE: Due to the sandboxing restrictions <String>.split() is forbidden... so
//  here's our own implementation!
List splitString(String s, String delim_expr=/\s/) {
  List out = []
  current = ""
  for (c in s) {
    if (c =~ delim_expr) {
      if (current.size() > 0) {
        out << current
        current = ""
      }
    } else {
      current += c
    }
  }
  if (current.size() > 0) {
    out << current
  }
  return out
}

//// Build Options /////////////////////////////////////////////////////////////

// Always Run:
//  * Build Stage
//  * Unit Test Stage
//
// Release Run:
//  * Version Stage
//  * COO Stage

def buildStage = true
def unitTestStage = true
def versionStage = false
def cooStage = false

def params = [
  string(defaultValue: nodeLabel,
    description: "Node to run job on",
    name: "NODE_LABEL"),
]
if (env.BRANCH_NAME == "master") {
  params << choice(choices: versionBumpChoices,
    description: versionBumpDescription,
    name: "VERSION_BUMP_TYPE")
  params << text(name: "CHANGE_LOG",
    defaultValue: "CHANGE LOG:",
    description: "Fill in information about the changes made in this new version")
}
properties([parameters(params)])
nodeLabel = env.NODE_LABEL

changeLog = ""
versionBumpType = ""
if (env.VERSION_BUMP_TYPE != "NONE") {
  versionStage = true
  cooStage = true
  versionBumpType = env.VERSION_BUMP_TYPE
  changeLog = env.CHANGE_LOG
}

print """Stage Configuration:
  * build: """ + buildStage + """
  * unitTest: """ + unitTestStage + """
  * version: """ + versionStage + """
  * coo: """ + cooStage

//// Stages ////////////////////////////////////////////////////////////////////

node(nodeLabel) {
  withCredentials( [usernameColonPassword(credentialsId: 'github-oauth-userpass', variable: 'GITHUB_USERPASS') ] ) {

    // Clean up anything lingering
    stage('Prepare Workspace') {
      deleteDir()
    }

    // Clone the repo and submodules
    stage('Clone') {
      checkout([
        $class: 'GitSCM', 
        branches: [[name: '*/${BRANCH_NAME}']], 
        doGenerateSubmoduleConfigurations: false, 
        extensions: [[
            $class: 'SubmoduleOption',
            disableSubmodules: false,
            recursiveSubmodules: false,
            parentCredentials: true,
            reference: '',
            trackingSubmodules: false
        ]],
        submoduleCfg: [],
        userRemoteConfigs: [[
            credentialsId: 'github-oauth-userpass',
            url: projectRepo
        ]]
      ])
      sh 'ci/wip_check.sh'
    }

    // Determine if this is a WIP and remaining stages should be skipped
    //  case-insensitive match for WIP branches, eg. 'wip-*' and 'WIP_*'
    if (fileExists('wip.tmp') || env.BRANCH_NAME ==~ /^(?i)wip(-|_).*$/ ) {

      echo 'WIP detected. Skipping remaining stages.'
      // set the build status to unstable
      currentBuild.result = 'UNSTABLE'
    } else {

      try {
        // Check whether or not the prebuilt dependency image is available. If not,
        // built it first so it can be used later.
        stage('Dependencies') {
          sh 'ci/dependency_stage.sh ' + baseImageVersion + ' ' + depsImageVersion
        }

        // From here out, everything runs in the container
        def dockerImage = docker.image(dockerRepo + "/" + depsImageName + ":" + depsImageVersion)
        dockerImage.inside(dockerArgs + " -v $WORKSPACE:/home/workspace") {

          if (buildStage) {
            stage('Build') {
              sh 'ci/setup_ghenkins_https.sh'
              sh 'cd /home/workspace && make'
            }
          }

          if (unitTestStage) {
            stage('Unit Test') {
              sh 'cd /home/workspace && make test || true'
              sh 'mkdir -p testlogs && cp -r /home/workspace/bazel-testlogs/* testlogs/'
              junit '**/testlogs/**/test.xml'
              archiveArtifacts '**/testlogs/**/test.log'
            }
          }

          if (versionStage) {
            stage('Versioning') {

              cmd = 'cd /home/workspace && ci/bump_version.sh'
              cmd += ' --user ghenkins'
              cmd += ' --bump-type ' + versionBumpType
              cmd += ' --change-log "' + changeLog + '"'
              sh cmd
            }
          }

          // NOTE: Something about the implementation of the csar() function
          // causes the workspace to be cleaned out, so this MUST be the last
          // stage (or the first, but it can't come between building and
          // versioning).
          if (cooStage) {
            stage('COO Prep') {
              csar()
            }
          }
        }
      } catch (e) {
        sh './ci/get_slack_handle.sh > slack_handle.txt'
        def output = readFile('slack_handle.txt').trim()
        if (output.size() > 0) {
          splits = splitString(output)
          slackHandle = splits[0]
          name = splits[1]
          slackSend(channel: "#alchemyapi-beegees", message: "@" + slackHandle + ": Hi " + name + """! You broke the build!
Branch: """ + env.BRANCH_NAME + """
Repo: """ + projectRepo + """
Build Link: """ + env.BUILD_URL)
        }
        throw e
      }
    }
  }
}

