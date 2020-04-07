#!/bin/bash

openssl aes-256-cbc -K $encrypted_729343b004dc_key -iv $encrypted_729343b004dc_iv -in $ENCRYPTED_GPG_KEY_LOCATION -out library/$GPG_KEY_LOCATION -d

./gradlew :library:publishReleasePublicationToMavenRepository
