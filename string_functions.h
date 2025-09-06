

#pragma once 

#define MAX_TOPIC_LEVELS 100  // Maximum levels in the topic hierarchy

String split_data[MAX_TOPIC_LEVELS];  // Global array to store topic levels
int topicCount = 0;  // Number of extracted levels

void splitTopic(String topic , String split_char) {
    char topicCopy[1000]; 
    strncpy(topicCopy, topic.c_str(), sizeof(topicCopy) - 1);
    topicCopy[sizeof(topicCopy) - 1] = '\0';  // Ensure null termination

    char* token = strtok(topicCopy, split_char.c_str());
    topicCount = 0;  // Reset count

    // Serial.println(MAX_TOPIC_LEVELS);

    while (token != NULL && topicCount < MAX_TOPIC_LEVELS) {
        split_data[topicCount] = String(token);
        token = strtok(NULL, split_char.c_str() );
        topicCount++;
    }
}
int gettopiccount(){
    return topicCount;
}