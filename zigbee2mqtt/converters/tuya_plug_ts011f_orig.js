import * as m from 'zigbee-herdsman-converters/lib/modernExtend';

export default {
    fingerprint: [
                    {manufacturerName: '_TZ3000_w0qqde0g'},
                    {manufacturerName: '_TZ3000_gjnozsaz'},
                    {manufacturerName: '_TZ3000_zloso4jk'}
                 ],
    zigbeeModel: ['TS011F'],
    model: 'Original Tuya glug ready for update',
    vendor: 'Slacky-DIY',
    description: 'Original Tuya plug ready for custom Firmware update',
    extend: [
                    m.identify()
            ],
    meta: {},
    ota: true,
};
