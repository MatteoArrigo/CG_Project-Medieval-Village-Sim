#pragma once
#include "character.hpp"
#include "Utils.hpp"
#include <vector>
#include <memory>
#include <glm/glm.hpp>

// Create comments for the CharManager class and its methods following the style of the other custom classes in the include folder.
/**
 * Manages a collection of Character instances.
 * Provides functionalities to add characters, find the nearest character to a given position,
 * and initialize characters from a scene file.
 */
class CharManager {
public:
    /**
     * Adds a Character to the manager's collection.
     * @param character character to be added.
     */
    void addChar(const std::shared_ptr<Character>& character) {
        characters.push_back(character);
    }

    /**
     * Finds the nearest Character in "Idle" state to the given player position within a specified maximum distance.
     * @param playerPos The current position of the player.
     * @return A shared pointer to the nearest Character in "Idle" state within maxDistance, or nullptr if none found.
     */
    std::shared_ptr<Character> getNearestCharacter(const glm::vec3& playerPos) const {
        std::shared_ptr<Character> nearest = nullptr;
        float minDist = maxDistance;
        for (const auto& charac : characters) {
            float dist = glm::distance(charac->getPosition(), playerPos);
            if (dist < minDist && charac->getCurrentState() == "Idle") {
                minDist = dist;
                nearest = charac;
            }
        }
        return nearest;
    }
    /**
     * Returns the collection of managed Character instances.
     */

    const std::vector<std::shared_ptr<Character>>& getCharacters() const {
        return characters;
    }

    /**
     * Initializes characters from a json (scene) file.
     */
    int init(std::string file, Scene SC) {
        AssetFile** af = SC.As;
        nlohmann::json sceneJson;
        std::ifstream ifs(file);
        if (!ifs.is_open()) {
            std::cout << "Error! Character file >" << file << "< not found!";
            exit(-1);
        }
        try {
            ifs >> sceneJson;
            ifs.close();
        } catch (const nlohmann::json::exception& e) {
            std::cout << "\n\n\nException while parsing JSON file: " << file << "\n";
            std::cout << e.what() << '\n' << '\n';
            std::cout << std::flush;
            return 1;
        }
        if (!sceneJson.contains("characters") || !sceneJson["characters"].is_array()) return -1;

        int skinId = 0; // Default skin ID, can be modified if needed

        // Creates a map string (asset file id) -> index in the AssetFile array
        std::unordered_map<std::string, int> assetFileIdToIndex;
        int assetFileIdx = 0;
        for (const auto& assetFile : sceneJson["assetfiles"]) {
            if (assetFile.contains("id")) {
                assetFileIdToIndex.insert({assetFile["id"].get<std::string>(), assetFileIdx});
            }
            assetFileIdx++;
        }

        // Creates a map string (instance id) -> Instance*
        std::unordered_map<std::string, Instance*> instanceIdToInstanceRef;
        for (auto kv : SC.InstanceIds) {
            Instance *inst = SC.I[kv.second];
            instanceIdToInstanceRef.insert({kv.first, inst});
        }

        for (const auto& charJson : sceneJson["characters"]) {
            std::string name = charJson.value("name", "Unknown");
            std::vector<std::string> instancesIds = charJson.value("instanceIds", std::vector<std::string>{});
            std::vector<float> posArr {0, 0, 0}; // Default position
            // Il la prima instance definisce la posizione del Character
            std::string posId = instancesIds[0];
            // Get position from the scene file (cook-torranceChar instances)
            for (const auto& techniqueJson : sceneJson["instances"]) {
                for (const auto& elementJson : techniqueJson["elements"]) {
                    if (elementJson["id"] == posId) {
                        if (elementJson.contains("translate"))
                            posArr = elementJson["translate"].get<std::vector<float>>();
                        break;
                    }
                }
            }

            glm::vec3 pos = glm::vec3(posArr[0], posArr[1], posArr[2]);
            auto animList = charJson.value("animList", std::vector<std::string>{});
            auto assetFilesList = sceneJson["assetfiles"];
            auto charStates = charJson.value("charStates", std::vector<std::string>{});
            auto dialogues = charJson.value("dialogues", std::vector<std::string>{});
            auto startEndFrames = charJson.value("startEndFrames", std::vector<std::vector<int>>{});
            std::string baseTrack = charJson.value("BaseTrackName", "");

            // Animations initialization
            int animCount = static_cast<int>(animList.size());
            // Initializes Anims[skinId] as a vector of Animations of size animCount
            if (Anims.size() <= skinId) {
                Anims.resize(skinId + 1);
            }
            Anims[skinId].resize(animCount);
            for (int ian = 0; ian < animCount; ian++) {

                // Find the corresponding AssetFile for the animation ID
                const std::string& idToSeek = animList[ian];
                if (assetFileIdToIndex.find(idToSeek) != assetFileIdToIndex.end()) {
                    assetFileIdx = assetFileIdToIndex[idToSeek];
                } else {
                    std::cout << "Error! Animation ID >" << idToSeek << "< not found in asset files list.\n";
                    return -1;
                }

                Anims[skinId][ian].init(*af[assetFileIdx]);
                std::cout << "ANIM " << ian << " of char " << skinId << " initialized with asset file: " << assetFilesList[assetFileIdx]["id"] << "\n";
            }

            // AnimBlender initialization
            std::vector<AnimBlendSegment> segments(startEndFrames.size());
            for (size_t i = 0; i < startEndFrames.size(); ++i) {
                segments[i] = {startEndFrames[i][0], startEndFrames[i][1], 0.0f, static_cast<int>(i)};
                // std::cout << "Starting frame: " << startEndFrames[i][0] << " Ending frame: " << startEndFrames[i][1] <<"\n";
            }
            auto ab = std::make_shared<AnimBlender>();
            ab->init(segments);
            // std::cout << "First segment starts at " << ab->segments[0].st << " and ends at " << ab->segments[0].en << "\n";


            // SkeletalAnimation initialization
            auto SKA = std::make_shared<SkeletalAnimation>();
            SKA->init(Anims[skinId].data(), animCount, baseTrack);
            skinId++;

            // Creates the Character and adds it to the manager
            auto charac = std::make_shared<Character>(name, pos, ab, SKA, charStates);
            if (!dialogues.empty()) {
				for(int i=0 ; i<dialogues.size(); i++)
					dialogues[i] = wrapText(dialogues[i], 25);
                charac->setDialogues(dialogues);
            }
            addChar(charac);

            // Adding instance references to the character
            std::vector<Instance*> charInstances;
            for (const auto& instanceId : instancesIds) {
                if (instanceIdToInstanceRef.find(instanceId) != instanceIdToInstanceRef.end()) {
                    charInstances.push_back(instanceIdToInstanceRef.at(instanceId));
                    std::cout << "Instance with ID: " << instanceId << " added to character: " << name << "\n";
                } else {
                    std::cout << "Instance with ID: " << instanceId << " not found in scene.\n";
                }
            }
            charac->setInstances(charInstances);
        }
        std::cout << "Characters initialization finished \n";
        return 0;
    }

    /**
     * Returns the vector of Animations for cleanup purposes.
     */
    std::vector<Animations>& getAnims() {
         return Anims[0]; // Ritorna l'ultima animazione caricata
    }

    /**
     * Cleans up all animations associated with the characters.
     */
    void cleanup() {
        for (int i = 0; i < Anims.size(); ++i) {
            for (int j = 0; j < Anims[i].size(); ++j) {
                Anims[i][j].cleanup();
            }
            // Anims[i].cleanup();
        }
    }

    /**
     * Returns the maximum distance to consider a Character "near".
     */
    float getMaxDistance() const {
        return maxDistance;
    }



private:
    /**
     * Collection of managed Character instances.
     */
    std::vector<std::shared_ptr<Character>> characters;

    /**
     * 2D vector of Animations for cleanup purposes.
     * Outer vector indexed by skin ID, inner vector contains Animations for that skin.
     */
    std::vector<std::vector<Animations>> Anims;

    /**
     * Maximum distance to consider a Character "near" for interaction.
     */
    constexpr static float maxDistance = 5.0f;
};