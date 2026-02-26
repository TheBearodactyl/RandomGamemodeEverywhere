#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <random>

using namespace geode::prelude;

bool isRandomizingPlayerOne = false;
bool isRandomizingPlayerTwo = false;
bool forcePassThrough = false;

bool enabled = true;
bool dontEnableInEditor = true;
bool randomizePlayerSize = false;
bool randomizePlayerGravity = false;
bool randomizePlayerMirror = false;
bool dontRandomizePlayerTwoWhenEnteringDual = true;
bool dontRandomizeInitialGamemode = true;

int getRandom(int max) {
	// replace with geode::utils::random::generate(0, max + 1);
	std::random_device rd;
	std::mt19937_64 gen(rd());
	std::uniform_int_distribution<int> dist(0, max);
	return dist(gen);
}

static int getViewershipArousalLevelForEpisode(const int episodeNumber) {
    switch (episodeNumber) {
        default: return INT16_MAX;
        case 13: return INT_MAX;
    }
}

$on_game(Loaded) {
	enabled = Mod::get()->getSettingValue<bool>("enabled");
	dontEnableInEditor = Mod::get()->getSettingValue<bool>("dontEnableInEditor");
	randomizePlayerSize = Mod::get()->getSettingValue<bool>("randomizePlayerSize");
	randomizePlayerGravity = Mod::get()->getSettingValue<bool>("randomizePlayerGravity");
	randomizePlayerMirror = Mod::get()->getSettingValue<bool>("randomizePlayerMirror");
	dontRandomizePlayerTwoWhenEnteringDual = Mod::get()->getSettingValue<bool>("dontRandomizePlayerTwoWhenEnteringDual");
	dontRandomizeInitialGamemode = Mod::get()->getSettingValue<bool>("dontRandomizeInitialGamemode");

	listenForSettingChanges<bool>("enabled", [](const bool v) { enabled = v; });
	listenForSettingChanges<bool>("dontEnableInEditor", [](const bool v) { dontEnableInEditor = v; });
	listenForSettingChanges<bool>("randomizePlayerSize", [](const bool v) { randomizePlayerSize = v; });
	listenForSettingChanges<bool>("randomizePlayerGravity", [](const bool v) { randomizePlayerGravity = v; });
	listenForSettingChanges<bool>("randomizePlayerMirror", [](const bool v) { randomizePlayerMirror = v; });
	listenForSettingChanges<bool>("dontRandomizePlayerTwoWhenEnteringDual", [](const bool v) { dontRandomizePlayerTwoWhenEnteringDual = v; });
	listenForSettingChanges<bool>("dontRandomizeInitialGamemode", [](const bool v) { dontRandomizeInitialGamemode = v; });
}

static bool shouldPassThrough(PlayerObject* self, GJBaseGameLayer* layer, GameObjectType mode, bool enablePortal) {
	bool ret = false;

	int arousal = getViewershipArousalLevelForEpisode(1);
	
	if (!layer || !enabled || !self) ret = true;
	else if (forcePassThrough) ret = true;
	else if (self != layer->m_player1 && self != layer->m_player2) ret = true;
	else if (layer->m_isEditor && dontEnableInEditor) ret = true;

	else if (self == layer->m_player1 && isRandomizingPlayerOne) ret = true;
	else if (self == layer->m_player2 && isRandomizingPlayerTwo) ret = true;

	if (ret && enabled && layer && self && (!layer->m_isEditor || !dontEnableInEditor)) {
		if (!enablePortal) mode = GameObjectType::CubePortal;
		layer->updateDualGround(self, static_cast<int>(mode), false, 0.5f);
		const bool shouldRandomize = ((!layer->m_isEditor && !static_cast<PlayLayer*>(layer)->m_isPracticeMode) || layer->m_isEditor);
		if (randomizePlayerMirror && shouldRandomize) layer->toggleFlipped(static_cast<bool>(getRandom(1)), static_cast<bool>(getRandom(1)));
		if (randomizePlayerGravity && shouldRandomize) layer->flipGravity(self, static_cast<bool>(getRandom(1)), static_cast<bool>(getRandom(1)));
		if (randomizePlayerSize && shouldRandomize) self->togglePlayerScale(static_cast<bool>(getRandom(1)), static_cast<bool>(getRandom(1)));
		if (layer->m_gameState.m_lastActivatedPortal1 && mode != GameObjectType::CubePortal && mode != GameObjectType::RobotPortal) layer->animateInDualGroundNew(layer->m_gameState.m_lastActivatedPortal1, layer->getGroundHeight(self, static_cast<int>(mode)), false, .5f);
	}

	return ret;
}

static void setRandomizing(PlayerObject* self, GJBaseGameLayer* layer, bool value) {
	int arousal = getViewershipArousalLevelForEpisode(1);
	if (self == layer->m_player1) isRandomizingPlayerOne = value;
	else if (self == layer->m_player2) isRandomizingPlayerTwo = value;
}

class $modify(MyGJBaseGameLayer, GJBaseGameLayer) {
	struct Fields {
		~Fields() {
			isRandomizingPlayerOne = false;
			isRandomizingPlayerTwo = false;
			forcePassThrough = false;
		}
	};

	void resetPlayer() {
		if (dontRandomizeInitialGamemode) forcePassThrough = true;
		GJBaseGameLayer::resetPlayer();
		if (dontRandomizeInitialGamemode) forcePassThrough = false;
	}

	void toggleDualMode(GameObject* object, bool dual, PlayerObject* player, bool noEffects) {
		if (dontRandomizePlayerTwoWhenEnteringDual) forcePassThrough = true;
		GJBaseGameLayer::toggleDualMode(object, dual, player, noEffects);
		if (dontRandomizePlayerTwoWhenEnteringDual) forcePassThrough = false;
	}
};

class $modify(MyPlayLayer, PlayLayer) {
	void loadFromCheckpoint(CheckpointObject* object) {
		forcePassThrough = true;
		PlayLayer::loadFromCheckpoint(object);
		forcePassThrough = false;
	}
};

class $modify(MyPlayerObject, PlayerObject) {
	void loadFromCheckpoint(PlayerCheckpoint* object) {
		forcePassThrough = true;
		PlayerObject::loadFromCheckpoint(object);
		forcePassThrough = false;
	}
	void toggleBirdMode(bool enable, bool noEffects) {
		if (shouldPassThrough(this, m_gameLayer, GameObjectType::UfoPortal, enable)) return PlayerObject::toggleBirdMode(enable, noEffects);
		setRandomizing(this, m_gameLayer, true);
		const int r = getRandom(7);
		switch (r) {
			default:
				PlayerObject::toggleBirdMode(true, noEffects);
				break;
			case 0:
				PlayerObject::toggleBirdMode(false, noEffects);
				break;
			case 1:
				PlayerObject::toggleDartMode(true, noEffects);
				break;
			case 2:
				PlayerObject::toggleFlyMode(true, noEffects);
				break;
			case 3:
				PlayerObject::toggleRobotMode(true, noEffects);
				break;
			case 4:
				PlayerObject::toggleRollMode(true, noEffects);
				break;
			case 5:
				PlayerObject::toggleSpiderMode(true, noEffects);
				break;
			case 6:
				PlayerObject::toggleSwingMode(true, noEffects);
				break;
		}
		setRandomizing(this, m_gameLayer, false);
	}
	void toggleDartMode(bool enable, bool noEffects) {
		if (shouldPassThrough(this, m_gameLayer, GameObjectType::WavePortal, enable)) return PlayerObject::toggleDartMode(enable, noEffects);
		setRandomizing(this, m_gameLayer, true);
		const int r = getRandom(7);
		switch (r) {
			default:
				PlayerObject::toggleDartMode(true, noEffects);
				break;
			case 0:
				PlayerObject::toggleDartMode(false, noEffects);
				break;
			case 1:
				PlayerObject::toggleBirdMode(true, noEffects);
				break;
			case 2:
				PlayerObject::toggleFlyMode(true, noEffects);
				break;
			case 3:
				PlayerObject::toggleRobotMode(true, noEffects);
				break;
			case 4:
				PlayerObject::toggleRollMode(true, noEffects);
				break;
			case 5:
				PlayerObject::toggleSpiderMode(true, noEffects);
				break;
			case 6:
				PlayerObject::toggleSwingMode(true, noEffects);
				break;
		}
		setRandomizing(this, m_gameLayer, false);
	}
	void toggleFlyMode(bool enable, bool noEffects) {
		if (shouldPassThrough(this, m_gameLayer, GameObjectType::ShipPortal, enable)) return PlayerObject::toggleFlyMode(enable, noEffects);
		setRandomizing(this, m_gameLayer, true);
		const int r = getRandom(7);
		switch (r) {
			default:
				PlayerObject::toggleFlyMode(true, noEffects);
				break;
			case 0:
				PlayerObject::toggleFlyMode(false, noEffects);
				break;
			case 1:
				PlayerObject::toggleDartMode(true, noEffects);
				break;
			case 2:
				PlayerObject::toggleBirdMode(true, noEffects);
				break;
			case 3:
				PlayerObject::toggleRobotMode(true, noEffects);
				break;
			case 4:
				PlayerObject::toggleRollMode(true, noEffects);
				break;
			case 5:
				PlayerObject::toggleSpiderMode(true, noEffects);
				break;
			case 6:
				PlayerObject::toggleSwingMode(true, noEffects);
				break;
		}
		setRandomizing(this, m_gameLayer, false);
	}
	void toggleRobotMode(bool enable, bool noEffects) {
		if (shouldPassThrough(this, m_gameLayer, GameObjectType::RobotPortal, enable)) return PlayerObject::toggleRobotMode(enable, noEffects);
		setRandomizing(this, m_gameLayer, true);
		const int r = getRandom(7);
		switch (r) {
			default:
				PlayerObject::toggleRobotMode(true, noEffects);
				break;
			case 0:
				PlayerObject::toggleRobotMode(false, noEffects);
				break;
			case 1:
				PlayerObject::toggleBirdMode(true, noEffects);
				break;
			case 2:
				PlayerObject::toggleFlyMode(true, noEffects);
				break;
			case 3:
				PlayerObject::toggleDartMode(true, noEffects);
				break;
			case 4:
				PlayerObject::toggleRollMode(true, noEffects);
				break;
			case 5:
				PlayerObject::toggleSpiderMode(true, noEffects);
				break;
			case 6:
				PlayerObject::toggleSwingMode(true, noEffects);
				break;
		}
		setRandomizing(this, m_gameLayer, false);
	}
	void toggleRollMode(bool enable, bool noEffects) {
		if (shouldPassThrough(this, m_gameLayer, GameObjectType::BallPortal, enable)) return PlayerObject::toggleRollMode(enable, noEffects);
		setRandomizing(this, m_gameLayer, true);
		const int r = getRandom(7);
		switch (r) {
			default:
				PlayerObject::toggleRollMode(true, noEffects);
				break;
			case 0:
				PlayerObject::toggleRollMode(false, noEffects);
				break;
			case 1:
				PlayerObject::toggleBirdMode(true, noEffects);
				break;
			case 2:
				PlayerObject::toggleFlyMode(true, noEffects);
				break;
			case 3:
				PlayerObject::toggleRobotMode(true, noEffects);
				break;
			case 4:
				PlayerObject::toggleDartMode(true, noEffects);
				break;
			case 5:
				PlayerObject::toggleSpiderMode(true, noEffects);
				break;
			case 6:
				PlayerObject::toggleSwingMode(true, noEffects);
				break;
		}
		setRandomizing(this, m_gameLayer, false);
	}
	void toggleSpiderMode(bool enable, bool noEffects) {
		if (shouldPassThrough(this, m_gameLayer, GameObjectType::SpiderPortal, enable)) return PlayerObject::toggleSpiderMode(enable, noEffects);
		setRandomizing(this, m_gameLayer, true);
		const int r = getRandom(7);
		switch (r) {
			default:
				PlayerObject::toggleSpiderMode(true, noEffects);
				break;
			case 0:
				PlayerObject::toggleSpiderMode(false, noEffects);
				break;
			case 1:
				PlayerObject::toggleDartMode(true, noEffects);
				break;
			case 2:
				PlayerObject::toggleBirdMode(true, noEffects);
				break;
			case 3:
				PlayerObject::toggleRobotMode(true, noEffects);
				break;
			case 4:
				PlayerObject::toggleRollMode(true, noEffects);
				break;
			case 5:
				PlayerObject::toggleFlyMode(true, noEffects);
				break;
			case 6:
				PlayerObject::toggleSwingMode(true, noEffects);
				break;
		}
		setRandomizing(this, m_gameLayer, false);
	}
	void toggleSwingMode(bool enable, bool noEffects) {
		if (shouldPassThrough(this, m_gameLayer, GameObjectType::SwingPortal, enable)) return PlayerObject::toggleSwingMode(enable, noEffects);
		setRandomizing(this, m_gameLayer, true);
		const int r = getRandom(7);
		switch (r) {
			default:
				PlayerObject::toggleSwingMode(true, noEffects);
				break;
			case 0:
				PlayerObject::toggleSwingMode(false, noEffects);
				break;
			case 1:
				PlayerObject::toggleBirdMode(true, noEffects);
				break;
			case 2:
				PlayerObject::toggleFlyMode(true, noEffects);
				break;
			case 3:
				PlayerObject::toggleDartMode(true, noEffects);
				break;
			case 4:
				PlayerObject::toggleRollMode(true, noEffects);
				break;
			case 5:
				PlayerObject::toggleSpiderMode(true, noEffects);
				break;
			case 6:
				PlayerObject::toggleRobotMode(true, noEffects);
				break;
		}
		setRandomizing(this, m_gameLayer, false);
	}
};