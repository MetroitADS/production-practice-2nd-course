const express = require('express');
const cors = require('cors');
const bodyParser = require('body-parser');
const { v4: uuidv4 } = require('uuid');
const fs = require('fs').promises;
const path = require('path');

const app = express();

const CONFIG_FILE = path.join(__dirname, 'config.json');
let config = {};

const loadConfig = async () => {
  try {
    const data = await fs.readFile(CONFIG_FILE, 'utf8');
    config = JSON.parse(data);
    return config;
  } catch (error) {
    console.error('Ошибка загрузки конфигурации:', error);
    config = {
      port: 3000,
      tokens: {
        "demo-token": ["read", "create", "update"]
      },
      eventsFile: "events.json",
      backupEnabled: true,
      backupDir: "backups"
    };
    return config;
  }
};

const PORT = process.env.PORT || 3000;
let EVENTS_FILE = 'events.json';

app.use(cors());
app.use(bodyParser.json());

const loadEvents = async () => {
  try {
    const data = await fs.readFile(EVENTS_FILE, 'utf8');
    return JSON.parse(data);
  } catch (error) {
    if (error.code === 'ENOENT') {
      return [];
    }
    console.error('Ошибка загрузки событий:', error);
    return [];
  }
};

const createBackup = async () => {
  if (!config.backupEnabled) return;
  try {
    await fs.mkdir(config.backupDir, { recursive: true });
    const backupFile = path.join(config.backupDir, `events.backup.${Date.now()}.json`);
    const currentEvents = await loadEvents();
    await fs.writeFile(backupFile, JSON.stringify(currentEvents, null, 2));
    try {
      const files = await fs.readdir(config.backupDir);
      const backupFiles = files
        .filter(file => file.startsWith('events.backup.') && file.endsWith('.json'))
        .sort()
        .reverse();
      if (backupFiles.length > 100) {
        const filesToDelete = backupFiles.slice(100);
        for (const file of filesToDelete) {
          await fs.unlink(path.join(config.backupDir, file));
          console.log(`🗑️ Удален старый backup: ${file}`);
        }
      }
    } catch (cleanupError) {
      console.warn('Не удалось очистить старые бэкапы:', cleanupError);
    }
  } catch (error) {
    console.warn('Не удалось создать backup:', error);
  }
};

const saveEvents = async (eventsData) => {
  try {
    await createBackup();
    await fs.writeFile(EVENTS_FILE, JSON.stringify(eventsData, null, 2), 'utf8');
  } catch (error) {
    console.error('Ошибка сохранения событий:', error);
    throw new Error('Не удалось сохранить события');
  }
};

const checkPermissions = (requiredPermissions) => {
  return (req, res, next) => {
    const authHeader = req.headers.authorization;
    if (!authHeader) {
      return res.status(401).json({ error: 'Требуется авторизация' });
    }
    const token = authHeader.replace('Bearer ', '');
    if (!config.tokens[token]) {
      return res.status(401).json({ error: 'Неверный токен авторизации' });
    }
    const userPermissions = config.tokens[token];
    const hasPermission = requiredPermissions.some(permission => 
      userPermissions.includes('*') || userPermissions.includes(permission)
    );
    if (!hasPermission) {
      return res.status(403).json({ 
        error: 'Недостаточно прав',
        required: requiredPermissions,
        has: userPermissions
      });
    }
    req.user = {
      token: token,
      permissions: userPermissions
    };
    next();
  };
};

app.get('/api/events', checkPermissions(['read']), async (req, res) => {
  try {
    const events = await loadEvents();
    res.json(events);
  } catch (error) {
    res.status(500).json({ error: 'Ошибка загрузки событий' });
  }
});

app.post('/api/events/sync', checkPermissions(['sync']), async (req, res) => {
  try {
    const clientEvents = req.body.events || [];
    
    const newEvents = clientEvents.map(event => ({
      id: event.id || uuidv4(),
      title: event.title,
      description: event.description,
      start: event.start,
      end: event.end,
      color: event.color
    }));
    await saveEvents(newEvents);
    res.json({ 
      message: 'События синхронизированы', 
      count: newEvents.length 
    });
  } catch (error) {
    res.status(500).json({ error: 'Ошибка синхронизации' });
  }
});

app.post('/api/events', checkPermissions(['create']), async (req, res) => {
  try {
    const events = await loadEvents();
    
    const event = {
      id: uuidv4(),
      title: req.body.title,
      description: req.body.description,
      start: req.body.start,
      end: req.body.end,
      color: req.body.color || '#3498db'
    };
    events.push(event);
    await saveEvents(events);
    res.status(201).json(event);
  } catch (error) {
    res.status(500).json({ error: 'Ошибка создания события' });
  }
});

app.put('/api/events/:id', checkPermissions(['update']), async (req, res) => {
  try {
    const events = await loadEvents();
    const eventId = req.params.id;
    const eventIndex = events.findIndex(e => e.id === eventId);
    if (eventIndex === -1) {
      return res.status(404).json({ error: 'Событие не найдено' });
    }
    events[eventIndex] = { ...events[eventIndex], ...req.body };
    await saveEvents(events);
    res.json(events[eventIndex]);
  } catch (error) {
    res.status(500).json({ error: 'Ошибка обновления события' });
  }
});

app.delete('/api/events/:id', checkPermissions(['delete']), async (req, res) => {
  try {
    const events = await loadEvents();
    const eventId = req.params.id;
    const filteredEvents = events.filter(e => e.id !== eventId);
    if (filteredEvents.length === events.length) {
      return res.status(404).json({ error: 'Событие не найдено' });
    }
    await saveEvents(filteredEvents);
    res.json({ message: 'Событие удалено' });
  } catch (error) {
    res.status(500).json({ error: 'Ошибка удаления события' });
  }
});

app.get('/api/events/date/:date', checkPermissions(['read']), async (req, res) => {
  try {
    const events = await loadEvents();
    const targetDate = req.params.date;
    const dateEvents = events.filter(event => 
      event.start && event.start.startsWith(targetDate)
    );
    res.json(dateEvents);
  } catch (error) {
    res.status(500).json({ error: 'Ошибка получения событий' });
  }
});

app.get('/api/info', checkPermissions(['read']), async (req, res) => {
  try {
    const events = await loadEvents();
    
    res.json({
      name: 'Calendar Sync Server',
      version: '1.0.0',
      eventsCount: events.length,
      storage: `Файловая система (${EVENTS_FILE})`,
      userPermissions: req.user.permissions,
      endpoints: [
        'GET /api/events - Получить все события (требует: read)',
        'POST /api/events/sync - Синхронизировать события (требует: sync)',
        'POST /api/events - Добавить событие (требует: create)',
        'PUT /api/events/:id - Обновить событие (требует: update)',
        'DELETE /api/events/:id - Удалить событие (требует: delete)',
        'GET /api/events/date/:date - События на дату (требует: read)'
      ]
    });
  } catch (error) {
    res.status(500).json({ error: 'Ошибка получения информации' });
  }
});

app.get('/api/permissions', checkPermissions(['*']), (req, res) => {
  res.json({
    availablePermissions: ['read', 'create', 'update', 'delete', 'sync', '*'],
    tokens: Object.keys(config.tokens).map(token => ({
      token: token,
      permissions: config.tokens[token],
      maskedToken: token.substring(0, 4) + '...' + token.substring(token.length - 4)
    }))
  });
});

const startServer = async () => {
  await loadConfig();
  EVENTS_FILE = path.join(__dirname, config.eventsFile || 'events.json');
  app.listen(PORT, async () => {
    console.log(`Сервер запущен на http://localhost:${PORT}`);
    console.log(`Доступные эндпоинты:`);
    console.log(`GET  /api/info - Информация о сервере`);
    console.log(`GET  /api/events - Получить все события`);
    console.log(`POST /api/events/sync - Синхронизация`);
    console.log(`\n Доступные токены:`);
    Object.entries(config.tokens).forEach(([token, permissions]) => {
      console.log(`   ${token.substring(0, 8)}... - ${permissions.join(', ')}`);
    });
    console.log(`События сохраняются в файл: ${EVENTS_FILE}`);
    console.log(`Backup система: ${config.backupEnabled ? 'включена' : 'выключена'}`);
    console.log(`Папка для бэкапов: ${config.backupDir || 'backups'}`);
    try {
      const events = await loadEvents();
      console.log(`Загружено событий: ${events.length}`);
    } catch (error) {
      console.log('Создан новый файл для хранения событий');
    }
  });
};

startServer().catch(console.error);